/**
 * @file    bsp_led.h
 * @brief   LEDs de usuario da NUCLEO-F767ZI (LD1 verde, LD2 azul, LD3 vermelho).
 *
 * @note    Estes sao os LEDs de sinalizacao da placa. O LED do digipot
 *          (acoplado ao LDR) e acionado pelo DAC: ver @ref BSP_DAC.
 *
 * @defgroup BSP_LED LEDs
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdbool.h>

/** @brief Identificadores dos LEDs da placa. */
typedef enum
{
    BSP_LED_GREEN = 0, /**< LD1 (PB0)  - "vivo" (heartbeat)                      */
    BSP_LED_BLUE,      /**< LD2 (PB7)  - LUT calibrada / malha de ohms travada   */
    BSP_LED_RED,       /**< LD3 (PB14) - ultimo comando terminou em erro         */
    BSP_LED_COUNT
} bsp_led_t;

/** @brief Apaga todos os LEDs. */
void bsp_led_init(void);

/** @brief Liga (@c true) ou desliga (@c false) um LED. */
void bsp_led_write(bsp_led_t led, bool on);

/** @brief Inverte o estado de um LED. */
void bsp_led_toggle(bsp_led_t led);

#endif /* BSP_LED_H */
/** @} */
