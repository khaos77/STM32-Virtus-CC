/**
 * @file    app.c
 * @brief   Aplicacao de teste: dispara testes pelo botao e valida o que recebe.
 * @ingroup APP
 *
 * @details
 * ### Padroes de dados
 * - **Padrao A** (teste minimo): `dados[i] = i`, i = 0..99.
 * - **Padrao B** (estresse, quadro k): `dados[0] = k`, `dados[i] = k + 13*i` (mod 256).
 *
 * O receptor confere o conteudo de ponta a ponta reconhecendo um dos dois
 * padroes. Isso detecta ate um erro que, por azar, passasse pelo CRC.
 *
 * ### Por que o receptor so imprime ao fim da rajada
 * O @c printf pela UART bloqueia o laco principal (~87 us por caractere).
 * Se o receptor imprimisse a cada quadro, demoraria a responder o ACK. Por
 * isso ele acumula os resultados e imprime quando o link fica em silencio
 * por ::APP_RX_BURST_END_MS.
 */
#include "app.h"
#include "app_config.h"
#include "bsp.h"
#include "gpio_link.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Tipos e estado                                                            */
/* ------------------------------------------------------------------------- */

/** @brief Padroes reconhecidos pelo receptor. */
typedef enum
{
    PATTERN_UNKNOWN = 0, /**< Conteudo nao confere */
    PATTERN_A,           /**< 0,1,2,...,99 */
    PATTERN_B            /**< k, k+13, k+26, ... */
} pattern_t;

/** @brief Acumulador de uma rajada de quadros recebidos. */
typedef struct
{
    bool         active;          /**< Rajada em andamento */
    uint32_t     last_ms;         /**< Instante do ultimo quadro */
    uint32_t     frames;          /**< Quadros entregues */
    uint32_t     content_errors;  /**< Quadros com conteudo divergente */
    link_stats_t stats0;          /**< Contadores do link no inicio */
    uint8_t      last_buf[LINK_MAX_PAYLOAD]; /**< Ultimo quadro (para exibir) */
    uint8_t      last_len;        /**< Tamanho do ultimo quadro */
    uint8_t      last_seq;        /**< SEQ do ultimo quadro */
    pattern_t    last_pattern;    /**< Padrao do ultimo quadro */
} rx_burst_t;

static rx_burst_t s_burst;
static uint8_t    s_tx_buf[LINK_MAX_PAYLOAD];
static uint8_t    s_rx_buf[LINK_MAX_PAYLOAD];

/* ------------------------------------------------------------------------- */
/* Utilitarios                                                               */
/* ------------------------------------------------------------------------- */

/** @brief Preenche o padrao A. */
static void fill_pattern_a(uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        buf[i] = i;
    }
}

/** @brief Preenche o padrao B para o quadro @p k. */
static void fill_pattern_b(uint8_t *buf, uint8_t len, uint8_t k)
{
    buf[0] = k;
    for (uint8_t i = 1; i < len; i++)
    {
        buf[i] = (uint8_t)(k + APP_STRESS_PATTERN_STEP * i);
    }
}

/** @brief Identifica o padrao de um bloco recebido. */
static pattern_t check_pattern(const uint8_t *buf, uint8_t len)
{
    bool a = true;
    bool b = (len > 0u);

    for (uint8_t i = 0; i < len; i++)
    {
        if (buf[i] != i)
        {
            a = false;
        }
        if ((i > 0u) && (buf[i] != (uint8_t)(buf[0] + APP_STRESS_PATTERN_STEP * i)))
        {
            b = false;
        }
    }
    if (a)
    {
        return PATTERN_A;
    }
    return b ? PATTERN_B : PATTERN_UNKNOWN;
}

/** @brief Taxa util em bit/s (payload apenas). */
static uint32_t throughput_bps(uint32_t bytes, uint32_t us)
{
    return (us == 0u) ? 0u : (uint32_t)(((uint64_t)bytes * 8u * 1000000u) / us);
}

/** @brief Imprime "parte/total" como porcentagem com 2 casas decimais. */
static void print_percent(uint32_t part, uint32_t total)
{
    const uint32_t x = (total == 0u) ? 0u : (uint32_t)(((uint64_t)part * 10000u) / total);
    printf("%lu.%02lu%%", (unsigned long)(x / 100u), (unsigned long)(x % 100u));
}

/** @brief Imprime um vetor em linhas de 20 valores. */
static void print_array(const uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        printf("%3u%s", buf[i], ((i % 20u) == 19u || i == (uint8_t)(len - 1u)) ? "\r\n" : " ");
    }
}

/** @brief Diferenca de um campo entre dois snapshots de estatisticas. */
#define DELTA(after, before, field) ((unsigned long)((after).field - (before).field))

/* ------------------------------------------------------------------------- */
/* Testes (lado transmissor - "modulo A")                                    */
/* ------------------------------------------------------------------------- */

/** @brief Teste minimo do roteiro: vetor de 100 bytes 0..99 de A para B. */
static void test_minimum(void)
{
    link_report_t rep;

    fill_pattern_a(s_tx_buf, APP_TEST_LEN);
    link_set_fault_injection(0u);

    printf("\r\n[TX] Teste minimo: enviando %u bytes {0..%u}...\r\n",
           APP_TEST_LEN, APP_TEST_LEN - 1u);

    bsp_led_off(BSP_LED_OK);
    bsp_led_off(BSP_LED_ERR);
    bsp_led_on(BSP_LED_TX);
    const link_status_t st = link_send(s_tx_buf, APP_TEST_LEN, &rep);
    bsp_led_off(BSP_LED_TX);

    if (st == LINK_OK)
    {
        bsp_led_on(BSP_LED_OK);
        printf("[TX] OK: ACK recebido.\r\n");
    }
    else
    {
        bsp_led_on(BSP_LED_ERR);
        printf("[TX] FALHA: sem ACK apos %u tentativas (verifique fios e GND).\r\n", rep.attempts);
    }
    printf("[TX] tentativas=%u  NACKs=%u  timeouts=%u\r\n", rep.attempts, rep.nacks, rep.timeouts);
    printf("[TX] tempo ate o ACK: %lu us (linha teorica do quadro: %lu us)\r\n",
           (unsigned long)rep.elapsed_us, (unsigned long)link_frame_time_us(APP_TEST_LEN));
    printf("[TX] taxa util: %lu bit/s (taxa bruta da linha: %lu bit/s)\r\n",
           (unsigned long)throughput_bps(APP_TEST_LEN, rep.elapsed_us),
           (unsigned long)(1000000u / (2u * LINK_T_HALF_US)));
}

/**
 * @brief Teste de estresse: ::APP_STRESS_FRAMES quadros de 100 bytes.
 * @param fault_percent Porcentagem de quadros corrompidos de proposito.
 */
static void test_stress(uint8_t fault_percent)
{
    link_stats_t s0;
    link_stats_t s1;
    link_report_t rep;
    uint32_t ok = 0u;
    uint32_t fail = 0u;
    uint32_t sum_attempts = 0u;
    uint32_t max_us = 0u;

    printf("\r\n[TX] Teste de estresse: %u quadros x %u bytes, injecao de falhas = %u%%\r\n",
           APP_STRESS_FRAMES, APP_TEST_LEN, fault_percent);

    link_get_stats(&s0);
    link_set_fault_injection(fault_percent);
    bsp_led_off(BSP_LED_OK);
    bsp_led_off(BSP_LED_ERR);

    uint32_t total_us = 0u;     /* acumulado por quadro: DWT da a volta em ~19,9 s */

    for (uint32_t k = 0; k < APP_STRESS_FRAMES; k++)
    {
        fill_pattern_b(s_tx_buf, APP_TEST_LEN, (uint8_t)k);

        bsp_led_on(BSP_LED_TX);
        const link_status_t st = link_send(s_tx_buf, APP_TEST_LEN, &rep);
        bsp_led_off(BSP_LED_TX);

        total_us     += rep.elapsed_us;
        sum_attempts += rep.attempts;
        if (rep.elapsed_us > max_us)
        {
            max_us = rep.elapsed_us;
        }
        if (st == LINK_OK)
        {
            ok++;
        }
        else
        {
            fail++;
            bsp_led_on(BSP_LED_ERR);
        }

        if (((k + 1u) % 100u) == 0u)
        {
            printf("[TX]  %4lu/%u  entregues=%lu  falhas=%lu\r\n",
                   (unsigned long)(k + 1u), APP_STRESS_FRAMES, (unsigned long)ok, (unsigned long)fail);
        }
    }

    link_set_fault_injection(0u);
    link_get_stats(&s1);

    printf("[TX] ===== Resultado do estresse =====\r\n");
    printf("[TX] entregues com ACK : %lu/%u (", (unsigned long)ok, APP_STRESS_FRAMES);
    print_percent(ok, APP_STRESS_FRAMES);
    printf(")\r\n");
    printf("[TX] falhas definitivas: %lu\r\n", (unsigned long)fail);
    printf("[TX] erros injetados   : %lu\r\n", DELTA(s1, s0, tx_faults_injected));
    printf("[TX] NACKs recebidos   : %lu\r\n", DELTA(s1, s0, tx_nacks_received));
    printf("[TX] timeouts          : %lu\r\n", DELTA(s1, s0, tx_timeouts));
    printf("[TX] retransmissoes    : %lu  (media %lu.%02lu tentativas/quadro)\r\n",
           DELTA(s1, s0, tx_retries),
           (unsigned long)(sum_attempts / APP_STRESS_FRAMES),
           (unsigned long)(((sum_attempts % APP_STRESS_FRAMES) * 100u) / APP_STRESS_FRAMES));
    printf("[TX] tempo total       : %lu ms  (medio %lu us/quadro, pior %lu us)\r\n",
           (unsigned long)(total_us / 1000u),
           (unsigned long)(total_us / APP_STRESS_FRAMES), (unsigned long)max_us);
    printf("[TX] taxa util media   : %lu bit/s\r\n",
           (unsigned long)throughput_bps(ok * APP_TEST_LEN, total_us));

    bsp_led_on((fail == 0u) ? BSP_LED_OK : BSP_LED_ERR);
}

/**
 * @brief Mede quanto tempo o botao fica pressionado e escolhe o teste.
 *
 * LED azul acende ao passar de 1 s; LED vermelho ao passar de 3 s,
 * para o usuario saber quando soltar. O link continua sendo atendido.
 */
static void handle_button(void)
{
    const uint32_t t0 = HAL_GetTick();
    uint32_t held = 0u;

    bsp_led_off(BSP_LED_OK);
    bsp_led_off(BSP_LED_ERR);

    while (bsp_button_is_pressed())
    {
        link_poll();
        held = HAL_GetTick() - t0;
        bsp_led_set(BSP_LED_OK,  held >= APP_LONG_PRESS_MS);
        bsp_led_set(BSP_LED_ERR, held >= APP_VERY_LONG_PRESS_MS);
    }
    bsp_led_off(BSP_LED_OK);
    bsp_led_off(BSP_LED_ERR);

    if (held >= APP_VERY_LONG_PRESS_MS)
    {
        test_stress(APP_STRESS_FAULT_PERCENT);
    }
    else if (held >= APP_LONG_PRESS_MS)
    {
        test_stress(0u);
    }
    else
    {
        test_minimum();
    }
}

/* ------------------------------------------------------------------------- */
/* Recepcao (lado receptor - "modulo B")                                     */
/* ------------------------------------------------------------------------- */

/** @brief Registra um bloco recebido na rajada atual. */
static void rx_account(const uint8_t *buf, uint8_t len, uint8_t seq)
{
    if (!s_burst.active)
    {
        memset(&s_burst, 0, sizeof(s_burst));
        s_burst.active = true;
        link_get_stats(&s_burst.stats0);
        bsp_led_off(BSP_LED_OK);
        bsp_led_off(BSP_LED_ERR);
    }

    const pattern_t p = check_pattern(buf, len);
    s_burst.frames++;
    if (p == PATTERN_UNKNOWN)
    {
        s_burst.content_errors++;
        bsp_led_on(BSP_LED_ERR);
    }
    bsp_led_toggle(BSP_LED_OK);              /* "pisca" a cada quadro */

    memcpy(s_burst.last_buf, buf, len);
    s_burst.last_len     = len;
    s_burst.last_seq     = seq;
    s_burst.last_pattern = p;
    s_burst.last_ms      = HAL_GetTick();
}

/** @brief Fecha a rajada apos o silencio e imprime o relatorio do receptor. */
static void rx_burst_report(void)
{
    link_stats_t s1;
    link_get_stats(&s1);
    s_burst.active = false;

    if (s_burst.frames == 1u)
    {
        printf("\r\n[RX] Quadro recebido: SEQ=%u, %u bytes, CRC ok\r\n",
               s_burst.last_seq, s_burst.last_len);
        print_array(s_burst.last_buf, s_burst.last_len);
        if (s_burst.last_pattern == PATTERN_A)
        {
            printf("[RX] Conteudo confere com {0..%u}: OK\r\n", (unsigned)(s_burst.last_len - 1u));
        }
        else if (s_burst.last_pattern == PATTERN_B)
        {
            printf("[RX] Conteudo confere com o padrao de estresse: OK\r\n");
        }
        else
        {
            printf("[RX] ATENCAO: conteudo nao reconhecido!\r\n");
        }
    }
    else
    {
        printf("\r\n[RX] ===== Rajada recebida =====\r\n");
        printf("[RX] quadros entregues     : %lu\r\n", (unsigned long)s_burst.frames);
        printf("[RX] conteudo divergente   : %lu\r\n", (unsigned long)s_burst.content_errors);
    }
    printf("[RX] CRC invalido (NACK)   : %lu\r\n", DELTA(s1, s_burst.stats0, rx_crc_errors));
    printf("[RX] duplicados descartados: %lu\r\n", DELTA(s1, s_burst.stats0, rx_duplicates));
    printf("[RX] quadros incompletos   : %lu\r\n", DELTA(s1, s_burst.stats0, rx_aborted));
    printf("[RX] glitches rejeitados   : %lu\r\n", DELTA(s1, s_burst.stats0, rx_glitches));

    bsp_led_off(BSP_LED_OK);
    bsp_led_on((s_burst.content_errors == 0u) ? BSP_LED_OK : BSP_LED_ERR);
}

/* ------------------------------------------------------------------------- */
/* API                                                                       */
/* ------------------------------------------------------------------------- */

void app_init(void)
{
    link_init();
    memset(&s_burst, 0, sizeof(s_burst));

    printf("\r\n==============================================\r\n");
    printf(" GPIO-Link - Lab GPIO/NVIC STM32 (Virtus-CC)\r\n");
    printf(" Criterio otimizado: ROBUSTEZ\r\n");
    printf("==============================================\r\n");
    printf(" Ligacao em cruz: D2(TX_CLK)->D4(RX_CLK), D3(TX_DATA)->D5(RX_DATA), GND-GND\r\n");
    printf(" Linha: %lu bit/s, quadro de %u bytes = %lu us\r\n",
           (unsigned long)(1000000u / (2u * LINK_T_HALF_US)), APP_TEST_LEN,
           (unsigned long)link_frame_time_us(APP_TEST_LEN));
    printf(" Botao azul: clique = teste minimo | 1-3 s = estresse | >3 s = estresse c/ falhas\r\n");
    printf(" Aguardando...\r\n");
}

void app_loop(void)
{
    uint8_t len;
    uint8_t seq;

    link_poll();

    if (bsp_button_take_event())
    {
        handle_button();
    }

    if (link_receive(s_rx_buf, &len, &seq))
    {
        rx_account(s_rx_buf, len, seq);
    }

    if (s_burst.active && ((HAL_GetTick() - s_burst.last_ms) >= APP_RX_BURST_END_MS))
    {
        rx_burst_report();
    }
}
