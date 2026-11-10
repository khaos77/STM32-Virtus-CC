/**
 * @file    bsp.c
 * @brief   Inicializacao dos modulos de placa da BSP.
 * @ingroup BSP
 */
#include "bsp.h"

void bsp_init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart_console)
{
    bsp_timing_init();
    bsp_console_init(huart_console);
    bsp_led_init();
    bsp_button_init();
    bsp_i2c_init(hi2c);
}
