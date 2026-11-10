/**
 * @file    bsp_console.h
 * @brief   Console serial via USART3 (Virtual COM do ST-LINK, 115200 8N1).
 *
 * @details - Transmissao: @c printf redirecionado (sobrescreve @c _write,
 *            que e @c weak em @c syscalls.c), bloqueante.
 *          - Recepcao: por interrupcao, byte a byte, armazenada em um buffer
 *            circular lido pelo superloop com bsp_console_read_char().
 *
 *          Abra um terminal serial (PuTTY, Tera Term, monitor serial do
 *          CubeIDE) na porta "STMicroelectronics STLink Virtual COM Port"
 *          a 115200 baud.
 *
 * @defgroup BSP_CONSOLE Console
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include "main.h"
#include <stdbool.h>

/** @brief Tamanho do buffer circular de recepcao (potencia de 2). */
#define BSP_CONSOLE_RX_BUF_SIZE 128U

/**
 * @brief Associa a UART, desliga o buffer de @c stdout e arma a recepcao
 *        por interrupcao.
 * @param huart UART ja inicializada pelo CubeMX.
 */
void bsp_console_init(UART_HandleTypeDef *huart);

/**
 * @brief Retira um caractere recebido, sem bloquear.
 * @param[out] ch Caractere lido (valido apenas se retornar @c true).
 * @return @c true se havia caractere disponivel.
 */
bool bsp_console_read_char(char *ch);

#endif /* BSP_CONSOLE_H */
/** @} */
