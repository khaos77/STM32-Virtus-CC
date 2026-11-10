/**
 * @file    bsp.h
 * @brief   Ponto de entrada da camada BSP (Board Support Package).
 *
 * @details O BSP isola TODO o acesso ao hardware (HAL/registradores) do resto
 *          do firmware. A aplicacao (pasta @c App) so enxerga as funcoes
 *          @c bsp_*, nunca @c HAL_* diretamente. Trocar de placa/timer/UART
 *          significa mexer apenas aqui.
 *
 *          Mapa de hardware (NUCLEO-F767ZI):
 *          | Funcao        | Pino | Periferico        | Conector          |
 *          |---------------|------|-------------------|-------------------|
 *          | PWM senoidal  | PA6  | TIM3_CH1 (AF2)    | CN7 pino 12 (D12) |
 *          | Console TX    | PD8  | USART3 (ST-LINK)  | USB do ST-LINK    |
 *          | Console RX    | PD9  | USART3 (ST-LINK)  | USB do ST-LINK    |
 *          | LED status    | PB0  | GPIO (LD1 verde)  | -                 |
 *          | LED erro      | PB14 | GPIO (LD3 vermelho)| -                |
 *
 * @defgroup BSP Board Support Package
 * @{
 */
#ifndef BSP_H
#define BSP_H

#include "main.h"
#include "bsp_led.h"
#include "bsp_pwm.h"
#include "bsp_console.h"

/**
 * @brief Inicializa todos os modulos do BSP.
 * @param htim  Timer de PWM ja configurado pelo CubeMX (TIM3).
 * @param huart UART do console ja configurada pelo CubeMX (USART3).
 * @note  Chamar depois dos @c MX_*_Init() gerados pelo CubeMX.
 */
void bsp_init(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart);

/**
 * @brief Tempo desde o reset, em milissegundos (base SysTick).
 * @return Contador de 32 bits (estoura a cada ~49 dias).
 */
uint32_t bsp_millis(void);

#endif /* BSP_H */
/** @} */
