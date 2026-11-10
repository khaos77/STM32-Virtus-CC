/**
 * @file    bsp_led.c
 * @brief   Acionamento dos LEDs de usuario.
 * @ingroup BSP_LED
 */
#include "bsp_led.h"
#include "main.h"

/** @brief Porta/pino de cada LED (rotulos definidos no CubeMX). */
static const struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
} s_leds[BSP_LED_COUNT] = {
    [BSP_LED_GREEN] = { LED_GREEN_GPIO_Port, LED_GREEN_Pin },
    [BSP_LED_BLUE]  = { LED_BLUE_GPIO_Port,  LED_BLUE_Pin  },
    [BSP_LED_RED]   = { LED_RED_GPIO_Port,   LED_RED_Pin   },
};

void bsp_led_init(void)
{
    for (int i = 0; i < BSP_LED_COUNT; i++)
    {
        HAL_GPIO_WritePin(s_leds[i].port, s_leds[i].pin, GPIO_PIN_RESET);
    }
}

void bsp_led_write(bsp_led_t led, bool on)
{
    if (led < BSP_LED_COUNT)
    {
        HAL_GPIO_WritePin(s_leds[led].port, s_leds[led].pin,
                          on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void bsp_led_toggle(bsp_led_t led)
{
    if (led < BSP_LED_COUNT)
    {
        HAL_GPIO_TogglePin(s_leds[led].port, s_leds[led].pin);
    }
}
