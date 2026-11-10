/**
 * @file    gpio_link.c
 * @brief   Implementacao do GPIO-Link: transmissao bit-banging e recepcao por EXTI.
 * @ingroup LINK
 *
 * @details
 * ### Divisao de trabalho
 * - **ISR (EXTI RX_CLK)**: monta bits em bytes e bytes em quadros. Faz o
 *   minimo possivel e nunca transmite nada.
 * - **link_poll() (laco principal)**: valida CRC, responde ACK/NACK, entrega
 *   dados a aplicacao.
 *
 * ### Buffer duplo (ping-pong) de recepcao
 * A ISR escreve sempre no buffer "work". Ao completar um quadro, se o laco
 * principal ja consumiu o anterior, os papeis sao trocados e o quadro pronto
 * fica no buffer "done". Assim ISR e laco principal nunca acessam o mesmo
 * buffer ao mesmo tempo, sem precisar copiar 255 bytes dentro da ISR.
 *
 * ### Maquina de estados de recepcao (por byte)
 * @verbatim
 *   SOF --0xAA--> TYPE --> SEQ --> LEN --(LEN>0)--> PAYLOAD --(idx==LEN)--> CRC --> [quadro pronto] --> SOF
 *    ^  (outro)                     \--(LEN==0)-------------------------->/
 *    +--- silencio > T_RESYNC entre bordas, ou quadro parado > RX_STALE (qualquer estado)
 * @endverbatim
 */
#include "gpio_link.h"
#include "crc8.h"
#include "bsp.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Tipos internos                                                            */
/* ------------------------------------------------------------------------- */

/** @brief Quadro bruto como chegou da linha (CRC ainda nao verificado). */
typedef struct
{
    uint8_t type;                       /**< Campo TYPE */
    uint8_t seq;                        /**< Campo SEQ */
    uint8_t len;                        /**< Campo LEN */
    uint8_t payload[LINK_MAX_PAYLOAD];  /**< Dados */
    uint8_t crc;                        /**< CRC recebido */
} link_raw_frame_t;

/** @brief Estados da maquina de recepcao. */
typedef enum
{
    RX_ST_SOF = 0, /**< Aguardando 0xAA */
    RX_ST_TYPE,    /**< Proximo byte e TYPE */
    RX_ST_SEQ,     /**< Proximo byte e SEQ */
    RX_ST_LEN,     /**< Proximo byte e LEN */
    RX_ST_PAYLOAD, /**< Recebendo dados */
    RX_ST_CRC      /**< Proximo byte e o CRC */
} rx_state_t;

/** @brief Contexto da recepcao (acessado pela ISR). */
typedef struct
{
    rx_state_t state;     /**< Estado atual */
    uint8_t    shift;     /**< Registrador de deslocamento do byte */
    uint8_t    bit_cnt;   /**< Bits ja recebidos do byte atual */
    uint16_t   idx;       /**< Indice no payload */
    uint8_t    work;      /**< Buffer que a ISR esta preenchendo (0/1) */
    uint32_t   last_edge; /**< Ciclo DWT da ultima borda aceita */
} rx_ctx_t;

/* ------------------------------------------------------------------------- */
/* Estado do modulo                                                          */
/* ------------------------------------------------------------------------- */

static rx_ctx_t           s_rx;                    /**< Contexto da ISR */
static link_raw_frame_t   s_rx_buf[2];             /**< Ping-pong */
static volatile uint8_t   s_rx_done_idx;           /**< Buffer com quadro pronto */
static volatile bool      s_rx_ready;              /**< Ha quadro pronto em s_rx_done_idx */

static volatile link_stats_t s_stats;              /**< Contadores */

static uint32_t s_resync_cycles;                   /**< LINK_T_RESYNC_US em ciclos */
static uint32_t s_stale_cycles;                    /**< LINK_RX_STALE_US em ciclos */

/* Entrega para a aplicacao (somente laco principal) */
static uint8_t  s_app_buf[LINK_MAX_PAYLOAD];
static uint8_t  s_app_len;
static uint8_t  s_app_seq;
static bool     s_app_ready;
static int16_t  s_rx_last_seq = -1;                /**< Ultimo SEQ entregue (-1 = nenhum) */

/* Resposta aguardada pelo transmissor (somente laco principal) */
static bool     s_resp_ready;
static uint8_t  s_resp_type;
static uint8_t  s_resp_seq;

/* Transmissao */
static uint8_t  s_tx_seq;
static bool     s_tx_seq_seeded;
static uint8_t  s_fault_percent;
static uint32_t s_prng = 0x12345678u;

/* ------------------------------------------------------------------------- */
/* Utilitarios                                                               */
/* ------------------------------------------------------------------------- */

/** @brief Gerador pseudoaleatorio xorshift32 (usado so na injecao de falhas). */
static uint32_t prng_next(void)
{
    uint32_t x = s_prng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_prng = x;
    return x;
}

/** @brief Volta a maquina de recepcao ao estado inicial. Chamar com IRQ protegida. */
static void rx_reset(void)
{
    s_rx.state   = RX_ST_SOF;
    s_rx.bit_cnt = 0u;
    s_rx.shift   = 0u;
    s_rx.idx     = 0u;
}

/** @brief CRC de um quadro (TYPE, SEQ, LEN, PAYLOAD). */
static uint8_t frame_crc(uint8_t type, uint8_t seq, uint8_t len, const uint8_t *payload)
{
    uint8_t crc = CRC8_INIT;
    crc = crc8_update(crc, type);
    crc = crc8_update(crc, seq);
    crc = crc8_update(crc, len);
    return crc8_compute(crc, payload, len);
}

/* ------------------------------------------------------------------------- */
/* Transmissao (bit-banging)                                                 */
/* ------------------------------------------------------------------------- */

/**
 * @brief Envia 1 bit: dado estavel por T_HALF antes e durante o nivel alto de CLK.
 */
static void tx_bit(uint8_t bit)
{
    bsp_link_io_tx_data_write(bit);          /* dado muda com CLK em 0      */
    bsp_timing_delay_us(LINK_T_HALF_US);     /* tempo de setup              */
    bsp_link_io_tx_clk_write(1u);            /* borda de subida -> EXTI     */
    bsp_timing_delay_us(LINK_T_HALF_US);     /* tempo de hold (amostragem)  */
    bsp_link_io_tx_clk_write(0u);
}

/** @brief Envia 1 byte, MSB primeiro. */
static void tx_byte(uint8_t b)
{
    for (int8_t i = 7; i >= 0; i--)
    {
        tx_bit((uint8_t)((b >> i) & 1u));
    }
}

/**
 * @brief Monta e transmite um quadro completo.
 * @param flip_idx Indice do payload a corromper (teste) ou -1 para nenhum.
 */
static void tx_frame(uint8_t type, uint8_t seq, const uint8_t *payload, uint8_t len, int16_t flip_idx)
{
    const uint8_t crc = frame_crc(type, seq, len, payload);

    bsp_timing_delay_us(LINK_T_IDLE_US);     /* silencio = delimitador de quadro */

    tx_byte(LINK_SOF);
    tx_byte(type);
    tx_byte(seq);
    tx_byte(len);
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t b = payload[i];
        if ((int16_t)i == flip_idx)
        {
            b ^= 0x01u;                      /* "ruido" artificial pos-CRC */
        }
        tx_byte(b);
    }
    tx_byte(crc);

    bsp_link_io_tx_data_write(0u);           /* linhas em repouso */
}

/* ------------------------------------------------------------------------- */
/* Recepcao (ISR)                                                            */
/* ------------------------------------------------------------------------- */

/** @brief Alimenta a maquina de estados com um byte completo (contexto de ISR). */
static void rx_push_byte(uint8_t b)
{
    link_raw_frame_t *w = &s_rx_buf[s_rx.work];

    switch (s_rx.state)
    {
    case RX_ST_SOF:
        if (b == LINK_SOF)
        {
            s_rx.state = RX_ST_TYPE;
        }
        break;

    case RX_ST_TYPE:
        w->type    = b;
        s_rx.state = RX_ST_SEQ;
        break;

    case RX_ST_SEQ:
        w->seq     = b;
        s_rx.state = RX_ST_LEN;
        break;

    case RX_ST_LEN:
        w->len     = b;
        s_rx.idx   = 0u;
        s_rx.state = (b > 0u) ? RX_ST_PAYLOAD : RX_ST_CRC;
        break;

    case RX_ST_PAYLOAD:
        w->payload[s_rx.idx++] = b;
        if (s_rx.idx >= w->len)
        {
            s_rx.state = RX_ST_CRC;
        }
        break;

    case RX_ST_CRC:
    default:
        w->crc = b;
        if (!s_rx_ready)
        {
            s_rx_done_idx = s_rx.work;       /* entrega este buffer...        */
            s_rx.work    ^= 1u;              /* ...e passa a escrever no outro */
            s_rx_ready    = true;
        }
        else
        {
            s_stats.rx_overruns++;           /* laco principal atrasado */
        }
        s_rx.state = RX_ST_SOF;
        break;
    }
}

/**
 * @brief ISR de borda de subida em RX_CLK: amostra 1 bit de RX_DATA.
 *
 * Executada em prioridade NVIC 0 (a mais alta), via
 * EXTI15_10_IRQHandler -> HAL_GPIO_EXTI_Callback -> bsp_link_io_rx_clk_irq.
 */
static void link_rx_clk_isr(void)
{
    const uint32_t now = bsp_timing_cycles();

    /* Filtro de glitch: uma borda legitima mantem CLK alto por T_HALF. */
    if (bsp_link_io_rx_clk_read() == 0u)
    {
        s_stats.rx_glitches++;
        return;
    }

    /* Votacao por maioria em 3 leituras de DATA. */
    uint8_t ones = bsp_link_io_rx_data_read();
    ones = (uint8_t)(ones + bsp_link_io_rx_data_read());
    ones = (uint8_t)(ones + bsp_link_io_rx_data_read());
    const uint8_t bit = (ones >= 2u) ? 1u : 0u;

    /* Silencio longo desde a ultima borda = comeco de um novo quadro. */
    if ((now - s_rx.last_edge) > s_resync_cycles)
    {
        if ((s_rx.state != RX_ST_SOF) || (s_rx.bit_cnt != 0u))
        {
            s_stats.rx_aborted++;            /* quadro anterior ficou incompleto */
        }
        rx_reset();
    }
    s_rx.last_edge = now;

    s_rx.shift = (uint8_t)((s_rx.shift << 1) | bit);
    if (++s_rx.bit_cnt < 8u)
    {
        return;
    }
    s_rx.bit_cnt = 0u;
    rx_push_byte(s_rx.shift);
}

/* ------------------------------------------------------------------------- */
/* Processamento no laco principal                                           */
/* ------------------------------------------------------------------------- */

/** @brief Valida um quadro recebido e executa a acao correspondente. */
static void process_frame(const link_raw_frame_t *f)
{
    const bool crc_ok = (frame_crc(f->type, f->seq, f->len, f->payload) == f->crc);

    if (!crc_ok)
    {
        s_stats.rx_crc_errors++;
        /* So pedimos retransmissao se nao era uma resposta (evita eco infinito). */
        if ((f->type != LINK_TYPE_ACK) && (f->type != LINK_TYPE_NACK))
        {
            tx_frame(LINK_TYPE_NACK, f->seq, NULL, 0u, -1);
            s_stats.nacks_sent++;
        }
        return;
    }

    switch (f->type)
    {
    case LINK_TYPE_DATA:
        /* Sempre confirma - inclusive duplicatas (o ACK anterior pode ter se perdido). */
        tx_frame(LINK_TYPE_ACK, f->seq, NULL, 0u, -1);
        s_stats.acks_sent++;

        if ((int16_t)f->seq == s_rx_last_seq)
        {
            s_stats.rx_duplicates++;
            return;
        }
        s_rx_last_seq = (int16_t)f->seq;

        if (s_app_ready)
        {
            s_stats.rx_overruns++;           /* aplicacao nao leu o anterior: sobrescreve */
        }
        memcpy(s_app_buf, f->payload, f->len);
        s_app_len   = f->len;
        s_app_seq   = f->seq;
        s_app_ready = true;
        s_stats.rx_frames_ok++;
        break;

    case LINK_TYPE_ACK:
    case LINK_TYPE_NACK:
        s_resp_type  = f->type;
        s_resp_seq   = f->seq;
        s_resp_ready = true;
        break;

    default:
        s_stats.rx_unexpected++;
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* API publica                                                               */
/* ------------------------------------------------------------------------- */

void link_init(void)
{
    memset((void *)&s_stats, 0, sizeof(s_stats));
    s_resync_cycles = bsp_timing_us_to_cycles(LINK_T_RESYNC_US);
    s_stale_cycles  = bsp_timing_us_to_cycles(LINK_RX_STALE_US);

    __disable_irq();
    rx_reset();
    s_rx.work      = 0u;
    s_rx.last_edge = bsp_timing_cycles();
    s_rx_ready     = false;
    __enable_irq();

    s_app_ready     = false;
    s_resp_ready    = false;
    s_rx_last_seq   = -1;
    s_tx_seq_seeded = false;
    s_fault_percent = 0u;

    bsp_link_io_tx_clk_write(0u);
    bsp_link_io_tx_data_write(0u);
    bsp_link_io_set_rx_clk_callback(link_rx_clk_isr);
}

void link_poll(void)
{
    /* 1) Descarta quadro parcial parado (ex.: cabo desconectado no meio). */
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (((s_rx.state != RX_ST_SOF) || (s_rx.bit_cnt != 0u)) &&
        ((bsp_timing_cycles() - s_rx.last_edge) > s_stale_cycles))
    {
        rx_reset();
        s_stats.rx_aborted++;
    }
    __set_PRIMASK(primask);

    /* 2) Processa o quadro completo, se houver. */
    if (s_rx_ready)
    {
        process_frame(&s_rx_buf[s_rx_done_idx]);
        s_rx_ready = false;                  /* libera o buffer para a ISR */
    }
}

link_status_t link_send(const uint8_t *data, uint16_t len, link_report_t *report)
{
    link_report_t rep = {0};

    if ((len > LINK_MAX_PAYLOAD) || ((data == NULL) && (len > 0u)))
    {
        return LINK_ERR_PARAM;
    }

    /* SEQ inicial "aleatorio" (instante do 1o envio), para que um reset do
       transmissor dificilmente repita o ultimo SEQ visto pelo receptor. */
    if (!s_tx_seq_seeded)
    {
        s_tx_seq        = (uint8_t)(bsp_timing_cycles() >> 4);
        s_prng         ^= bsp_timing_cycles() | 1u;
        s_tx_seq_seeded = true;
    }

    const uint8_t  seq = s_tx_seq++;
    const uint32_t t0  = bsp_timing_cycles();
    link_status_t  status = LINK_ERR_NO_ACK;

    for (uint8_t attempt = 1u; attempt <= LINK_MAX_ATTEMPTS; attempt++)
    {
        rep.attempts = attempt;

        /* Injecao de falha (apenas em teste). */
        int16_t flip = -1;
        if ((s_fault_percent > 0u) && (len > 0u) && ((prng_next() % 100u) < s_fault_percent))
        {
            flip = (int16_t)(prng_next() % len);
            rep.faults_injected++;
            s_stats.tx_faults_injected++;
        }

        s_resp_ready = false;                /* ignora respostas antigas */
        tx_frame(LINK_TYPE_DATA, seq, data, (uint8_t)len, flip);
        s_stats.tx_data_frames++;
        if (attempt > 1u)
        {
            s_stats.tx_retries++;
        }

        /* Espera ACK/NACK atendendo o link. */
        enum { WAIT, GOT_ACK, GOT_NACK } result = WAIT;
        const uint32_t t_wait = HAL_GetTick();
        while ((result == WAIT) && ((HAL_GetTick() - t_wait) < LINK_ACK_TIMEOUT_MS))
        {
            link_poll();
            if (s_resp_ready)
            {
                s_resp_ready = false;
                if ((s_resp_type == LINK_TYPE_ACK) && (s_resp_seq == seq))
                {
                    result = GOT_ACK;
                }
                else if (s_resp_type == LINK_TYPE_NACK)
                {
                    result = GOT_NACK;       /* SEQ do NACK pode estar corrompido: aceita qualquer */
                }
                /* ACK de outro SEQ = resposta atrasada: continua esperando */
            }
        }

        if (result == GOT_ACK)
        {
            s_stats.tx_acks_received++;
            status = LINK_OK;
            break;
        }
        if (result == GOT_NACK)
        {
            s_stats.tx_nacks_received++;
            rep.nacks++;
        }
        else
        {
            s_stats.tx_timeouts++;
            rep.timeouts++;
        }
    }

    if (status != LINK_OK)
    {
        s_stats.tx_failed++;
    }

    rep.elapsed_us = bsp_timing_elapsed_us(t0);
    if (report != NULL)
    {
        *report = rep;
    }
    return status;
}

bool link_receive(uint8_t *buf, uint8_t *len, uint8_t *seq)
{
    if (!s_app_ready)
    {
        return false;
    }
    memcpy(buf, s_app_buf, s_app_len);
    *len = s_app_len;
    if (seq != NULL)
    {
        *seq = s_app_seq;
    }
    s_app_ready = false;
    return true;
}

void link_get_stats(link_stats_t *out)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memcpy(out, (const void *)&s_stats, sizeof(*out));
    __set_PRIMASK(primask);
}

void link_set_fault_injection(uint8_t percent)
{
    s_fault_percent = (percent > 100u) ? 100u : percent;
}

uint32_t link_frame_time_us(uint16_t len)
{
    return LINK_T_IDLE_US + ((uint32_t)len + LINK_OVERHEAD) * 8u * 2u * LINK_T_HALF_US;
}
