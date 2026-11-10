/**
 * @file    cli.c
 * @brief   Interpretador de comandos e receptor do quadro binario da LUT.
 * @ingroup CLI
 */
#include "cli.h"
#include "app_config.h"
#include "digipot.h"
#include "ohm_ctrl.h"
#include "bsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX_LEN   64U   /**< Tamanho maximo de uma linha de comando */
#define TEST_POINTS    10U   /**< Valores de res do roteiro de testes    */

/** @brief Estados da CLI. */
typedef enum
{
    CLI_STATE_TEXT = 0, /**< Lendo comandos de texto               */
    CLI_STATE_LOAD      /**< Recebendo o quadro binario da LUT     */
} cli_state_t;

static cli_state_t s_state;                          /**< Estado atual                  */
static char        s_line[LINE_MAX_LEN];             /**< Linha em edicao               */
static uint32_t    s_line_len;                       /**< Caracteres na linha           */
static uint8_t     s_frame[DIGIPOT_FRAME_DATA_SIZE]; /**< Corpo + CRC do quadro da LUT  */
static uint32_t    s_load_count;                     /**< Bytes do quadro ja recebidos (com magico) */
static uint32_t    s_load_start_ms;                  /**< Inicio da espera pelo quadro  */

/** @brief Mostra o prompt. */
static void prompt(void)
{
    printf("> ");
}

/** @brief Sinaliza o resultado do ultimo comando no LED vermelho. */
static void set_error(bool error)
{
    bsp_led_write(BSP_LED_RED, error);
}

/**
 * @brief Converte um argumento decimal.
 * @param[in]  s   Texto (pode ser @c NULL).
 * @param[out] out Valor convertido.
 * @return @c true se @p s e um numero decimal valido.
 */
static bool parse_u32(const char *s, uint32_t *out)
{
    char *end;

    if ((s == NULL) || (*s < '0') || (*s > '9'))
    {
        return false;
    }
    *out = (uint32_t)strtoul(s, &end, 10);
    return *end == '\0';
}

/** @brief Raiz quadrada inteira (piso). */
static uint32_t isqrt64(uint64_t v)
{
    uint64_t r = 0U;
    uint64_t bit = (uint64_t)1 << 62;

    while (bit > v)
    {
        bit >>= 2;
    }
    while (bit != 0U)
    {
        if (v >= (r + bit))
        {
            v -= r + bit;
            r  = (r >> 1) + bit;
        }
        else
        {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)r;
}

/** @brief Imprime uma medida em uma linha legivel. */
static void print_meas(const digipot_meas_t *m)
{
    printf("adc=%u  v=%lu mV  R_ldr=", (unsigned)m->adc_raw, (unsigned long)m->mv);
    if (m->r_ohm == DIGIPOT_R_OPEN)
    {
        printf("aberto");
    }
    else
    {
        printf("%lu ohm", (unsigned long)m->r_ohm);
    }
    printf("%s\r\n", m->valid ? "" : "  (fora da faixa util do ADC)");
}

/** @brief Comando @c help. */
static void cmd_help(void)
{
    printf("Comandos:\r\n"
           "  help              esta lista\r\n"
           "  info              estado atual\r\n"
           "  res <0-255>       aplica a palavra digital pela LUT\r\n"
           "  dac <0-4095>      escreve o codigo bruto do DAC\r\n"
           "  read              mede ADC, tensao e resistencia do LDR\r\n"
           "  sweep [passo]     curva R x res (CSV)\r\n"
           "  dsweep [pontos]   curva R x codigo DAC (CSV)\r\n"
           "  cal [rmin rmax]   autocalibra a LUT pelo ADC (~19 s)\r\n"
           "  load              recebe nova LUT pela UART/DMA (binario)\r\n"
           "  default           restaura a LUT padrao\r\n"
           "  lut               imprime a LUT\r\n"
           "  test              10 valores de res + RMSE\r\n"
           "  ohm <R>           mantem R ohms em malha fechada\r\n"
           "  stop              desliga a malha do comando ohm\r\n");
}

/** @brief Comando @c info. */
static void cmd_info(void)
{
    uint32_t r_min, r_max;

    digipot_range(&r_min, &r_max);
    printf("res=%u  dac=%u (%lu mV)  R_serie=%lu ohm\r\n",
           (unsigned)digipot_get(), (unsigned)digipot_get_dac(),
           (unsigned long)bsp_dac_code_to_mv(digipot_get_dac()),
           (unsigned long)APP_R_SERIES_OHM);
    if (digipot_is_calibrated())
    {
        printf("LUT calibrada: %lu a %lu ohm (res 0 a 255)\r\n",
               (unsigned long)r_min, (unsigned long)r_max);
    }
    else
    {
        printf("LUT padrao (nao calibrada): use 'cal' ou 'load'\r\n");
    }
    printf("malha ohm: %s  reinicios da recepcao: %lu\r\n",
           ohm_ctrl_active() ? "ligada" : "desligada",
           (unsigned long)bsp_console_rx_restarts());
}

/** @brief Comando @c lut: 8 entradas por linha. */
static void cmd_lut(void)
{
    const uint16_t *lut = digipot_lut();

    for (uint32_t i = 0U; i < DIGIPOT_LUT_SIZE; i++)
    {
        if ((i % 8U) == 0U)
        {
            printf("%3lu:", (unsigned long)i);
        }
        printf(" %4u", (unsigned)lut[i]);
        if ((i % 8U) == 7U)
        {
            printf("\r\n");
        }
    }
}

/**
 * @brief Comando @c sweep: curva "Resistencia_digipot x res".
 * @param step Passo de @c res entre pontos (>= 1).
 * @note  Varre de 255 para 0 (luz crescente), que e o sentido em que o LDR
 *        acomoda mais rapido.
 */
static void cmd_sweep(uint32_t step)
{
    uint8_t saved = digipot_get();
    int32_t res   = 255;
    bool    first = true;

    printf("# res;dac;adc;mV;R_medido_ohm;R_nominal_ohm;valido\r\n");
    for (;;)
    {
        digipot_meas_t m;

        digipot_set((uint8_t)res);
        bsp_delay_ms(first ? APP_SETTLE_FIRST_MS : APP_SETTLE_MS);
        first = false;
        digipot_measure(&m);
        printf("%ld;%u;%u;%lu;%lu;%lu;%u\r\n", (long)res, (unsigned)digipot_get_dac(),
               (unsigned)m.adc_raw, (unsigned long)m.mv, (unsigned long)m.r_ohm,
               (unsigned long)digipot_nominal_ohms((uint8_t)res), (unsigned)m.valid);

        if (res == 0)
        {
            break;
        }
        res -= (int32_t)step;
        if (res < 0)
        {
            res = 0;                /* garante o ponto res = 0 */
        }
    }
    printf("# fim\r\n");
    digipot_set(saved);
}

/**
 * @brief Comando @c dsweep: curva "Resistencia x codigo do DAC" (sem LUT).
 * @param points Numero de pontos entre o limiar do LED e 4095 (>= 2).
 */
static void cmd_dsweep(uint32_t points)
{
    const uint32_t span  = BSP_DAC_MAX_CODE - APP_DAC_LED_MIN_CODE;
    uint8_t        saved = digipot_get();

    printf("# dac;adc;mV;R_medido_ohm;valido\r\n");
    for (uint32_t i = 0U; i < points; i++)
    {
        digipot_meas_t m;
        uint16_t code = (uint16_t)(APP_DAC_LED_MIN_CODE + ((i * span) / (points - 1U)));

        digipot_set_dac(code);
        bsp_delay_ms((i == 0U) ? APP_SETTLE_FIRST_MS : APP_SETTLE_MS);
        digipot_measure(&m);
        printf("%u;%u;%lu;%lu;%u\r\n", (unsigned)code, (unsigned)m.adc_raw,
               (unsigned long)m.mv, (unsigned long)m.r_ohm, (unsigned)m.valid);
    }
    printf("# fim\r\n");
    digipot_set(saved);
}

/**
 * @brief Comando @c test: roteiro de testes do lab.
 *
 * Aplica @ref TEST_POINTS valores de @c res, mede cada um pelo ADC e calcula
 *
 * @f[ RMSE = \sqrt{\frac{1}{N}\sum_{i=1}^{N}\left(R_{medido,i}-R_{nominal,i}\right)^2} @f]
 */
static void cmd_test(void)
{
    uint8_t        saved = digipot_get();
    digipot_meas_t m[TEST_POINTS];
    uint64_t       sum_sq = 0U;
    uint32_t       n      = 0U;
    uint32_t       r_min, r_max;

    /* Mede de res alto para baixo (luz crescente)... */
    for (int32_t i = (int32_t)TEST_POINTS - 1; i >= 0; i--)
    {
        digipot_set((uint8_t)(i * 28));
        bsp_delay_ms((i == (int32_t)TEST_POINTS - 1) ? APP_SETTLE_FIRST_MS
                                                      : (2U * APP_SETTLE_MS));
        digipot_measure(&m[i]);
    }

    /* ...e imprime em ordem crescente. */
    printf("# res;R_nominal_ohm;R_adc_ohm;erro_ohm;valido\r\n");
    for (uint32_t i = 0U; i < TEST_POINTS; i++)
    {
        uint32_t nominal = digipot_nominal_ohms((uint8_t)(i * 28U));
        int64_t  err     = (int64_t)m[i].r_ohm - (int64_t)nominal;

        printf("%lu;%lu;%lu;%ld;%u\r\n", (unsigned long)(i * 28U),
               (unsigned long)nominal, (unsigned long)m[i].r_ohm,
               (long)err, (unsigned)m[i].valid);
        if (m[i].valid && digipot_is_calibrated())
        {
            sum_sq += (uint64_t)(err * err);
            n++;
        }
    }
    printf("# fim\r\n");

    if (n > 0U)
    {
        uint32_t rmse = isqrt64(sum_sq / n);

        digipot_range(&r_min, &r_max);
        printf("RMSE (ADC x nominal, %lu pontos) = %lu ohm (%lu.%lu %% da faixa)\r\n",
               (unsigned long)n, (unsigned long)rmse,
               (unsigned long)(((uint64_t)rmse * 100U) / (r_max - r_min)),
               (unsigned long)((((uint64_t)rmse * 1000U) / (r_max - r_min)) % 10U));
    }
    else
    {
        printf("LUT nao calibrada: sem resistencia nominal para o RMSE.\r\n");
    }
    printf("Para o relatorio, repita cada 'res <n>' medindo A-B com o multimetro.\r\n");
    digipot_set(saved);
}

/**
 * @brief Executa uma linha de comando completa.
 * @param line Linha terminada em @c '\0' (e modificada por @c strtok).
 */
static void execute(char *line)
{
    const char *cmd = strtok(line, " \t");
    const char *a1  = strtok(NULL, " \t");
    const char *a2  = strtok(NULL, " \t");
    uint32_t    v1  = 0U;
    uint32_t    v2  = 0U;
    bool        ok  = true;

    if (cmd == NULL)
    {
        return;
    }

    if (strcmp(cmd, "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        cmd_info();
    }
    else if (strcmp(cmd, "read") == 0)
    {
        digipot_meas_t m;
        digipot_measure(&m);
        print_meas(&m);
    }
    else if (strcmp(cmd, "lut") == 0)
    {
        cmd_lut();
    }
    else if (strcmp(cmd, "stop") == 0)
    {
        ohm_ctrl_stop();
        printf("malha desligada (dac=%u)\r\n", (unsigned)digipot_get_dac());
    }
    else if (strcmp(cmd, "res") == 0)
    {
        ok = parse_u32(a1, &v1) && (v1 <= 255U);
        if (ok)
        {
            ohm_ctrl_stop();
            digipot_set((uint8_t)v1);
            printf("res=%lu -> dac=%u", (unsigned long)v1, (unsigned)digipot_get_dac());
            if (digipot_is_calibrated())
            {
                printf("  R_nominal=%lu ohm", (unsigned long)digipot_nominal_ohms((uint8_t)v1));
            }
            printf("\r\n");
        }
    }
    else if (strcmp(cmd, "dac") == 0)
    {
        ok = parse_u32(a1, &v1) && (v1 <= BSP_DAC_MAX_CODE);
        if (ok)
        {
            ohm_ctrl_stop();
            digipot_set_dac((uint16_t)v1);
            printf("dac=%lu (%lu mV)\r\n", (unsigned long)v1,
                   (unsigned long)bsp_dac_code_to_mv((uint16_t)v1));
        }
    }
    else if (strcmp(cmd, "sweep") == 0)
    {
        v1 = 5U;
        ok = (a1 == NULL) || (parse_u32(a1, &v1) && (v1 >= 1U) && (v1 <= 255U));
        if (ok)
        {
            ohm_ctrl_stop();
            cmd_sweep(v1);
        }
    }
    else if (strcmp(cmd, "dsweep") == 0)
    {
        v1 = APP_CAL_POINTS;
        ok = (a1 == NULL) || (parse_u32(a1, &v1) && (v1 >= 2U) && (v1 <= 1024U));
        if (ok)
        {
            ohm_ctrl_stop();
            cmd_dsweep(v1);
        }
    }
    else if (strcmp(cmd, "cal") == 0)
    {
        ok = (a1 == NULL) || (parse_u32(a1, &v1) && parse_u32(a2, &v2) && (v1 < v2));
        if (ok)
        {
            digipot_status_t st;

            ohm_ctrl_stop();
            printf("Autocalibrando (%u pontos), aguarde...\r\n", (unsigned)APP_CAL_POINTS);
            st = digipot_autocal(v1, v2);
            if (st == DIGIPOT_OK)
            {
                digipot_range(&v1, &v2);
                printf("OK LUT calibrada: %lu a %lu ohm\r\n",
                       (unsigned long)v1, (unsigned long)v2);
            }
            else
            {
                printf("ERRO %s\r\n", digipot_status_str(st));
                set_error(true);
                return;
            }
        }
    }
    else if (strcmp(cmd, "load") == 0)
    {
        ohm_ctrl_stop();
        s_state         = CLI_STATE_LOAD;
        s_load_count    = 0U;
        s_load_start_ms = bsp_millis();
        printf("READY %u\r\n", (unsigned)DIGIPOT_FRAME_SIZE);
    }
    else if (strcmp(cmd, "default") == 0)
    {
        ohm_ctrl_stop();
        digipot_lut_default();
        printf("LUT padrao restaurada (nao calibrada)\r\n");
    }
    else if (strcmp(cmd, "test") == 0)
    {
        ohm_ctrl_stop();
        cmd_test();
    }
    else if (strcmp(cmd, "ohm") == 0)
    {
        ok = parse_u32(a1, &v1) && (v1 > 0U);
        if (ok)
        {
            ohm_ctrl_start(v1);
            printf("malha ligada: alvo=%lu ohm ('stop' para desligar)\r\n", (unsigned long)v1);
        }
    }
    else
    {
        printf("comando desconhecido: '%s' (digite 'help')\r\n", cmd);
        set_error(true);
        return;
    }

    if (!ok)
    {
        printf("argumento invalido (digite 'help')\r\n");
    }
    set_error(!ok);
}

/** @brief Encerra o estado de carga e volta aos comandos de texto. */
static void load_finish(void)
{
    s_state    = CLI_STATE_TEXT;
    s_line_len = 0U;
    prompt();
}

/**
 * @brief Estado @ref CLI_STATE_LOAD: sincroniza no magico e acumula o quadro.
 *
 * Bytes anteriores ao magico (por exemplo o @c '\\n' que segue o comando
 * @c load) sao descartados.
 */
static void task_load(void)
{
    uint8_t b;

    while (bsp_console_read_byte(&b))
    {
        if (s_load_count == 0U)
        {
            s_load_count = (b == DIGIPOT_FRAME_MAGIC0) ? 1U : 0U;
        }
        else if (s_load_count == 1U)
        {
            if (b == DIGIPOT_FRAME_MAGIC1)
            {
                s_load_count = 2U;
            }
            else
            {
                s_load_count = (b == DIGIPOT_FRAME_MAGIC0) ? 1U : 0U;
            }
        }
        else
        {
            s_frame[s_load_count - 2U] = b;
            s_load_count++;
            if (s_load_count == DIGIPOT_FRAME_SIZE)
            {
                digipot_status_t st = digipot_lut_load(s_frame);

                if (st == DIGIPOT_OK)
                {
                    uint32_t r_min, r_max;
                    digipot_range(&r_min, &r_max);
                    printf("OK LUT atualizada: %lu a %lu ohm\r\n",
                           (unsigned long)r_min, (unsigned long)r_max);
                }
                else
                {
                    printf("ERRO %s (LUT anterior mantida)\r\n", digipot_status_str(st));
                }
                set_error(st != DIGIPOT_OK);
                load_finish();
                return;
            }
        }
    }

    if ((bsp_millis() - s_load_start_ms) >= APP_LOAD_TIMEOUT_MS)
    {
        printf("ERRO timeout: %lu de %u bytes (LUT anterior mantida)\r\n",
               (unsigned long)s_load_count, (unsigned)DIGIPOT_FRAME_SIZE);
        set_error(true);
        bsp_console_flush_rx();
        load_finish();
    }
}

/** @brief Estado @ref CLI_STATE_TEXT: edicao de linha com eco. */
static void task_text(void)
{
    uint8_t b;

    while ((s_state == CLI_STATE_TEXT) && bsp_console_read_byte(&b))
    {
        if ((b == '\r') || (b == '\n'))
        {
            if (s_line_len == 0U)
            {
                continue;           /* linha vazia ou segundo byte de CR+LF */
            }
            printf("\r\n");
            s_line[s_line_len] = '\0';
            s_line_len         = 0U;
            execute(s_line);
            if (s_state == CLI_STATE_TEXT)
            {
                prompt();
            }
        }
        else if ((b == 0x08U) || (b == 0x7FU))      /* backspace / delete */
        {
            if (s_line_len > 0U)
            {
                s_line_len--;
                printf("\b \b");
            }
        }
        else if ((b >= 0x20U) && (b < 0x7FU) && (s_line_len < (LINE_MAX_LEN - 1U)))
        {
            s_line[s_line_len++] = (char)b;
            putchar((int)b);                        /* eco */
        }
        else
        {
            /* caractere de controle ou linha cheia: ignora */
        }
    }
}

void cli_init(void)
{
    s_state    = CLI_STATE_TEXT;
    s_line_len = 0U;
    bsp_console_flush_rx();
    prompt();
}

void cli_task(void)
{
    if (s_state == CLI_STATE_LOAD)
    {
        task_load();
    }
    else
    {
        task_text();
    }
}
