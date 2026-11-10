/**
 * @file    bsp_led.h
 * @brief   LEDs de usuario da NUCLEO-F767ZI.
 *
 * @defgroup BSP_LED LEDs
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_LED_H
#define BSP_LED_H

#include "main.h"
#include <stdbool.h>

/** @brief LEDs disponiveis. */
typedef enum
{
    BSP_LED_STATUS = 0, /**< LD1 (verde, PB0): pisca enquanto o firmware roda */
    BSP_LED_ERR,        /**< LD3 (vermelho, PB14): comando invalido na UART  */
    BSP_LED_COUNT       /**< Quantidade de LEDs (nao e um LED)               */
} bsp_led_t;

/** @brief Apaga todos os LEDs. */
void bsp_led_init(void);

/**
 * @brief Liga ou desliga um LED.
 * @param led LED alvo.
 * @param on  @c true liga, @c false desliga.
 */
void bsp_led_set(bsp_led_t led, bool on);

/**
 * @brief Inverte o estado de um LED.
 * @param led LED alvo.
 */
void bsp_led_toggle(bsp_led_t led);

#endif /* BSP_LED_H */
/** @} */
