/**
 * @file    bsp_console.h
 * @brief   Console serial via USART3 (Virtual COM do ST-LINK, 115200 8N1)
 *          com recepcao por DMA circular.
 *
 * @details - Transmissao: @c printf redirecionado (sobrescreve @c _write,
 *            que e @c weak em @c syscalls.c), bloqueante.
 *          - Recepcao: o DMA1_Stream1 copia cada byte que chega na USART3
 *            para um buffer circular de @ref BSP_CONSOLE_RX_BUF_SIZE bytes,
 *            sem interrupcao por byte. O superloop descobre quantos bytes
 *            chegaram lendo o contador do DMA (NDTR):
 *
 * @verbatim
 *   escrita (DMA)  = TAMANHO - NDTR
 *   leitura (app)  = s_tail
 *   disponiveis    = (escrita - leitura) mod TAMANHO
 * @endverbatim
 *
 *          O mesmo fluxo serve aos comandos de texto e ao quadro binario
 *          de 524 bytes que sobrescreve a look-up-table (requisito do lab:
 *          "sobrescrever a LUT pela UART, via DMA"). O buffer comporta o
 *          quadro inteiro, portanto nenhum byte se perde mesmo que o
 *          superloop esteja ocupado durante a recepcao.
 *
 * @defgroup BSP_CONSOLE Console (DMA)
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include "main.h"
#include <stdbool.h>

/** @brief Tamanho do buffer circular de recepcao (potencia de 2). */
#define BSP_CONSOLE_RX_BUF_SIZE 1024U

/**
 * @brief Associa a UART, desliga o buffer de @c stdout e inicia a recepcao
 *        por DMA circular.
 * @param huart UART ja inicializada pelo CubeMX (com DMA de RX vinculado).
 */
void bsp_console_init(UART_HandleTypeDef *huart);

/**
 * @brief Quantidade de bytes recebidos e ainda nao lidos.
 * @note  Tambem religa a recepcao caso a HAL a tenha abortado por erro de
 *        linha (overrun, ruido, framing).
 */
uint32_t bsp_console_available(void);

/**
 * @brief Retira um byte recebido, sem bloquear.
 * @param[out] byte Byte lido (valido apenas se retornar @c true).
 * @return @c true se havia byte disponivel.
 */
bool bsp_console_read_byte(uint8_t *byte);

/** @brief Descarta tudo o que ja foi recebido. */
void bsp_console_flush_rx(void);

/** @brief Numero de vezes que a recepcao precisou ser reiniciada por erro. */
uint32_t bsp_console_rx_restarts(void);

#endif /* BSP_CONSOLE_H */
/** @} */
