/**
 * @file    bsp_console.h
 * @brief   Console de log via USART3 (Virtual COM do ST-LINK, 115200 8N1).
 *
 * @details Redireciona @c printf para a UART sobrescrevendo @c _write()
 *          (que e @c weak em @c syscalls.c). Abra um terminal serial na porta
 *          "STMicroelectronics STLink Virtual COM Port" a 115200 baud.
 *
 * @defgroup BSP_CONSOLE Console
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include "main.h"

/**
 * @brief Associa a UART de log e desliga o buffer de @c stdout
 *        (cada @c printf sai imediatamente).
 * @param huart UART ja inicializada pelo CubeMX, ou @c NULL.
 */
void bsp_console_init(UART_HandleTypeDef *huart);

#endif /* BSP_CONSOLE_H */
/** @} */
