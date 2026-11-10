/**
 * @file    bsp.c
 * @brief   Inicializacao agregada do BSP.
 * @ingroup BSP
 */
#include "bsp.h"

void bsp_init(TIM_HandleTypeDef *htim, UART_HandleTypeDef *huart)
{
    bsp_led_init();
    bsp_console_init(huart);
    bsp_pwm_init(htim);
}

uint32_t bsp_millis(void)
{
    return HAL_GetTick();
}
