/**
 * @file    bsp.c
 * @brief   Inicializacao agregada da BSP e base de tempo.
 * @ingroup BSP
 */
#include "bsp.h"

bool bsp_init(ADC_HandleTypeDef *hadc, DAC_HandleTypeDef *hdac,
              UART_HandleTypeDef *huart)
{
    bool ok = true;

    bsp_led_init();
    bsp_console_init(huart);        /* primeiro: habilita o printf */
    ok = bsp_dac_init(hdac) && ok;
    ok = bsp_adc_init(hadc) && ok;
    return ok;
}

uint32_t bsp_millis(void)
{
    return HAL_GetTick();
}

void bsp_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
