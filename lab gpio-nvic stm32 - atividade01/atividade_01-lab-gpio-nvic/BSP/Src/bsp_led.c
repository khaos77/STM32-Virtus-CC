/**
 * @file    bsp_led.c
 * @brief   Implementacao dos LEDs usando HAL_GPIO.
 * @ingroup BSP_LED
 */
#include "bsp_led.h"
#include "main.h"

/** @brief Mapeamento LED logico -> porta/pino (labels definidos no CubeMX). */
typedef struct
{
    GPIO_TypeDef *port; /**< Porta GPIO */
    uint16_t      pin;  /**< Mascara do pino */
} bsp_led_map_t;

static const bsp_led_map_t s_led_map[BSP_LED_COUNT] =
{
    [BSP_LED_TX]  = { LED_TX_GPIO_Port,  LED_TX_Pin  },
    [BSP_LED_OK]  = { LED_OK_GPIO_Port,  LED_OK_Pin  },
    [BSP_LED_ERR] = { LED_ERR_GPIO_Port, LED_ERR_Pin },
};

void bsp_led_init(void)
{
    for (int i = 0; i < (int)BSP_LED_COUNT; i++)
    {
        HAL_GPIO_WritePin(s_led_map[i].port, s_led_map[i].pin, GPIO_PIN_RESET);
    }
}

void bsp_led_on(bsp_led_t led)
{
    if (led < BSP_LED_COUNT)
    {
        HAL_GPIO_WritePin(s_led_map[led].port, s_led_map[led].pin, GPIO_PIN_SET);
    }
}

void bsp_led_off(bsp_led_t led)
{
    if (led < BSP_LED_COUNT)
    {
        HAL_GPIO_WritePin(s_led_map[led].port, s_led_map[led].pin, GPIO_PIN_RESET);
    }
}

void bsp_led_toggle(bsp_led_t led)
{
    if (led < BSP_LED_COUNT)
    {
        HAL_GPIO_TogglePin(s_led_map[led].port, s_led_map[led].pin);
    }
}

void bsp_led_set(bsp_led_t led, int on)
{
    if (on)
    {
        bsp_led_on(led);
    }
    else
    {
        bsp_led_off(led);
    }
}
