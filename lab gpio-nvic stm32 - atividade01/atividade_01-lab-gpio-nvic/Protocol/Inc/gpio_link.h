/**
 * @file    gpio_link.h
 * @brief   GPIO-Link: protocolo serial sincrono, confiavel, sobre GPIO + EXTI.
 *
 * @details Protocolo projetado para trocar blocos de bytes entre duas placas
 *          STM32 usando apenas GPIO e interrupcao externa (sem UART/SPI/I2C).
 *          Criterio otimizado: **ROBUSTEZ** (maior taxa de entrega correta).
 *
 * ## Camada fisica
 * Duas linhas unidirecionais por sentido (CLK e DATA), ligadas em cruz entre
 * as placas. O transmissor gera o clock; o receptor amostra DATA na borda de
 * subida de CLK, dentro da ISR da EXTI. Por ser sincrono, o protocolo nao
 * depende de os dois clocks de CPU serem iguais.
 *
 * @verbatim
 *            |<-- T_HALF -->|<-- T_HALF -->|
 *   DATA  ===X=====  bit n  ===============X=== bit n+1
 *   CLK   ____________________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾\______
 *                             ^ receptor amostra (ISR EXTI)
 * @endverbatim
 *
 * ## Quadro (bytes enviados MSB primeiro)
 * @verbatim
 *   +------+------+-----+-----+-------------------+-------+
 *   | SOF  | TYPE | SEQ | LEN | PAYLOAD[0..LEN-1] | CRC-8 |
 *   | 0xAA | 1 B  | 1 B | 1 B |    0..255 B       |  1 B  |
 *   +------+------+-----+-----+-------------------+-------+
 *                 \__________ coberto pelo CRC ___________/ (exceto o proprio CRC)
 * @endverbatim
 *
 * ## Mecanismos de robustez
 * 1. **CRC-8** sobre TYPE, SEQ, LEN e PAYLOAD.
 * 2. **ACK/NACK + retransmissao** (stop-and-wait, ate ::LINK_MAX_ATTEMPTS).
 * 3. **Timeout** de resposta (::LINK_ACK_TIMEOUT_MS).
 * 4. **SEQ** para descartar quadros duplicados (ACK perdido).
 * 5. **Ressincronizacao por silencio**: intervalo entre bordas maior que
 *    ::LINK_T_RESYNC_US reinicia a maquina de estados (alinha os bytes).
 * 6. **Filtro de glitch**: borda so e aceita se CLK ainda estiver alto na ISR.
 * 7. **Votacao por maioria**: DATA e lido 3 vezes, vale o valor em >= 2 leituras.
 * 8. **Pull-down** nas entradas: linha solta = repouso, sem bordas falsas.
 *
 * @defgroup LINK GPIO-Link (Protocol)
 * @brief    Protocolo de enlace sobre GPIO + EXTI.
 * @{
 */
#ifndef GPIO_LINK_H
#define GPIO_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** @name Parametros de temporizacao (podem ser sobrescritos com -D)
 *  @{ */
#ifndef LINK_T_HALF_US
/** Meio periodo de bit em us. 5 us -> 10 us/bit -> 100 kbit/s brutos. */
#define LINK_T_HALF_US        5u
#endif
#ifndef LINK_T_IDLE_US
/** Silencio garantido antes de cada quadro (marca inicio de quadro). */
#define LINK_T_IDLE_US        100u
#endif
#ifndef LINK_T_RESYNC_US
/** Intervalo entre bordas acima do qual o receptor reinicia o quadro. */
#define LINK_T_RESYNC_US      40u
#endif
#ifndef LINK_RX_STALE_US
/** Quadro parcial parado ha mais que isso e descartado no link_poll(). */
#define LINK_RX_STALE_US      1000u
#endif
#ifndef LINK_ACK_TIMEOUT_MS
/** Tempo maximo de espera por ACK/NACK apos enviar um quadro. */
#define LINK_ACK_TIMEOUT_MS   50u
#endif
#ifndef LINK_MAX_ATTEMPTS
/** Tentativas de envio (1 original + retransmissoes). */
#define LINK_MAX_ATTEMPTS     5u
#endif
/** @} */

/** Tamanho maximo do payload (campo LEN tem 8 bits). */
#define LINK_MAX_PAYLOAD      255u
/** Delimitador de inicio de quadro. */
#define LINK_SOF              0xAAu
/** Bytes de overhead por quadro: SOF + TYPE + SEQ + LEN + CRC. */
#define LINK_OVERHEAD         5u

/** @brief Tipos de quadro (campo TYPE). */
typedef enum
{
    LINK_TYPE_DATA = 0x01, /**< Quadro de dados */
    LINK_TYPE_ACK  = 0x06, /**< Confirmacao positiva (ASCII ACK) */
    LINK_TYPE_NACK = 0x15  /**< Confirmacao negativa (ASCII NAK) */
} link_type_t;

/** @brief Resultado de ::link_send. */
typedef enum
{
    LINK_OK = 0,        /**< Quadro confirmado com ACK */
    LINK_ERR_NO_ACK,    /**< Esgotou as tentativas sem ACK */
    LINK_ERR_PARAM      /**< Parametro invalido */
} link_status_t;

/** @brief Relatorio de um envio (preenchido por ::link_send). */
typedef struct
{
    uint8_t  attempts;        /**< Tentativas usadas (1 = sem retransmissao) */
    uint8_t  nacks;           /**< NACKs recebidos */
    uint8_t  timeouts;        /**< Tentativas sem resposta */
    uint8_t  faults_injected; /**< Erros injetados de proposito (teste) */
    uint32_t elapsed_us;      /**< Tempo total ate o ACK (ou desistencia) */
} link_report_t;

/** @brief Contadores acumulados desde ::link_init. */
typedef struct
{
    /* lado transmissor */
    uint32_t tx_data_frames;     /**< Quadros DATA enviados (inclui retransmissoes) */
    uint32_t tx_retries;         /**< Retransmissoes */
    uint32_t tx_acks_received;   /**< ACKs validos recebidos */
    uint32_t tx_nacks_received;  /**< NACKs recebidos */
    uint32_t tx_timeouts;        /**< Esperas por resposta que expiraram */
    uint32_t tx_failed;          /**< Envios abandonados */
    uint32_t tx_faults_injected; /**< Erros injetados (teste) */
    /* lado receptor */
    uint32_t rx_frames_ok;       /**< Quadros DATA validos entregues */
    uint32_t rx_duplicates;      /**< Quadros repetidos descartados (re-ACK) */
    uint32_t rx_crc_errors;      /**< Quadros com CRC invalido */
    uint32_t rx_aborted;         /**< Quadros incompletos descartados */
    uint32_t rx_overruns;        /**< Quadros perdidos por buffer cheio */
    uint32_t rx_glitches;        /**< Bordas espurias rejeitadas */
    uint32_t rx_unexpected;      /**< Quadros validos de tipo desconhecido */
    uint32_t acks_sent;          /**< ACKs enviados */
    uint32_t nacks_sent;         /**< NACKs enviados */
} link_stats_t;

/**
 * @brief Inicializa o protocolo e registra a ISR de recepcao na BSP.
 * @pre   ::bsp_init ja chamada.
 */
void link_init(void);

/**
 * @brief Servico do protocolo - chamar continuamente no laco principal.
 *
 * Processa quadros completos recebidos pela ISR: verifica CRC, responde
 * ACK/NACK a quadros DATA, entrega dados novos para ::link_receive e
 * registra respostas aguardadas por ::link_send. Tambem descarta quadros
 * parciais abandonados.
 */
void link_poll(void);

/**
 * @brief Envia um bloco de forma confiavel (bloqueante, stop-and-wait).
 *
 * Envia o quadro DATA e aguarda ACK com o mesmo SEQ. Em NACK ou timeout,
 * retransmite ate ::LINK_MAX_ATTEMPTS vezes. Enquanto espera, continua
 * atendendo o link (::link_poll).
 *
 * @param data   Dados a enviar.
 * @param len    Quantidade de bytes (0..::LINK_MAX_PAYLOAD).
 * @param report Relatorio do envio (pode ser @c NULL).
 * @return ::LINK_OK se confirmado.
 */
link_status_t link_send(const uint8_t *data, uint16_t len, link_report_t *report);

/**
 * @brief Retira o ultimo bloco recebido, se houver.
 * @param buf Destino (minimo ::LINK_MAX_PAYLOAD bytes).
 * @param len Recebe o tamanho do bloco.
 * @param seq Recebe o numero de sequencia (pode ser @c NULL).
 * @return @c true se havia um bloco novo.
 */
bool link_receive(uint8_t *buf, uint8_t *len, uint8_t *seq);

/** @brief Copia os contadores atuais para @p out. */
void link_get_stats(link_stats_t *out);

/**
 * @brief Injeta erros de proposito (teste de robustez).
 *
 * Com probabilidade @p percent %, cada quadro DATA tem 1 bit do payload
 * invertido DEPOIS do calculo do CRC, simulando ruido na linha. O receptor
 * deve detectar via CRC e responder NACK.
 *
 * @param percent 0 desliga; 1..100.
 */
void link_set_fault_injection(uint8_t percent);

/**
 * @brief Tempo teorico de linha de um quadro com @p len bytes de payload (us).
 */
uint32_t link_frame_time_us(uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_LINK_H */
/** @} */
