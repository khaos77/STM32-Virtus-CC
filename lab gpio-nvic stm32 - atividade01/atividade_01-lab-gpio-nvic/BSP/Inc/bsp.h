/**
 * @file    bsp.h
 * @brief   Ponto de entrada unico da BSP (Board Support Package) da NUCLEO-F767ZI.
 *
 * @details A BSP e a unica camada que conhece pinos, portas e perifericos da
 *          placa. As camadas superiores (Protocol e App) so falam com o
 *          hardware atraves das funcoes declaradas nos headers incluidos aqui.
 *
 * @verbatim
 *   +-----------------------------+
 *   |  App      (app.c)           |  testes, LEDs de status, relatorios
 *   +-----------------------------+
 *   |  Protocol (gpio_link.c)     |  quadros, CRC-8, ACK/NACK, retransmissao
 *   +-----------------------------+
 *   |  BSP      (bsp_*.c)         |  GPIO/EXTI, DWT, LEDs, botao, console
 *   +-----------------------------+
 *   |  STM32 HAL + CubeMX (Core)  |
 *   +-----------------------------+
 * @endverbatim
 *
 * @defgroup BSP BSP - Board Support Package
 * @brief    Abstracao do hardware da NUCLEO-F767ZI usando a HAL.
 * @{
 */
#ifndef BSP_H
#define BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "bsp_timing.h"
#include "bsp_led.h"
#include "bsp_button.h"
#include "bsp_link_io.h"
#include "bsp_console.h"

/**
 * @brief Inicializa toda a BSP.
 *
 * Deve ser chamada uma vez, depois de @c MX_GPIO_Init() e
 * @c MX_USART3_UART_Init() (que sao geradas pelo CubeMX).
 *
 * @param huart_console UART usada para log (@c printf). Pode ser @c NULL
 *                      para desabilitar o console.
 */
void bsp_init(UART_HandleTypeDef *huart_console);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
/** @} */
