/**
 * @file    bsp_led.c
 * @brief   Implementacao dos LEDs de usuario.
 * @ingroup BSP_LED
 */
#include "bsp_led.h"

/** @brief Par porta/pino de um LED. */
typedef struct
{
    GPIO_TypeDef *port; /**< Porta GPIO */
    uint16_t      pin;  /**< Mascara do pino */
} bsp_led_io_t;

/** @brief Tabela LED -> pino (labels definidos no CubeMX / main.h). */
static const bsp_led_io_t s_leds[BSP_LED_COUNT] = {
    [BSP_LED_STATUS] = { LED_STATUS_GPIO_Port, LED_STATUS_Pin },
    [BSP_LED_ERR]    = { LED_ERR_GPIO_Port,    LED_ERR_Pin    },
};

void bsp_led_init(void)
{
    for (int i = 0; i < BSP_LED_COUNT; i++)
    {
        bsp_led_set((bsp_led_t)i, false);
    }
}

void bsp_led_set(bsp_led_t led, bool on)
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
