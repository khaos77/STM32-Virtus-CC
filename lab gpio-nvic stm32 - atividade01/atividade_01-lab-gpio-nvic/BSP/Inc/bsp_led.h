/**
 * @file    bsp_led.h
 * @brief   LEDs de usuario da NUCLEO-F767ZI.
 *
 * | LED        | Pino | Cor      | Uso no projeto                       |
 * |------------|------|----------|--------------------------------------|
 * | ::BSP_LED_TX  | PB0  | verde    | aceso enquanto transmite um quadro   |
 * | ::BSP_LED_OK  | PB7  | azul     | pacote entregue / recebido com sucesso |
 * | ::BSP_LED_ERR | PB14 | vermelho | falha (sem ACK, conteudo divergente) |
 *
 * @defgroup BSP_LED LEDs
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_LED_H
#define BSP_LED_H

/** @brief Identificador logico dos LEDs. */
typedef enum
{
    BSP_LED_TX = 0, /**< LD1 verde (PB0)     */
    BSP_LED_OK,     /**< LD2 azul (PB7)      */
    BSP_LED_ERR,    /**< LD3 vermelho (PB14) */
    BSP_LED_COUNT   /**< Quantidade de LEDs  */
} bsp_led_t;

/** @brief Apaga todos os LEDs. */
void bsp_led_init(void);
/** @brief Acende um LED. */
void bsp_led_on(bsp_led_t led);
/** @brief Apaga um LED. */
void bsp_led_off(bsp_led_t led);
/** @brief Inverte o estado de um LED. */
void bsp_led_toggle(bsp_led_t led);
/** @brief Acende (@p on != 0) ou apaga um LED. */
void bsp_led_set(bsp_led_t led, int on);

#endif /* BSP_LED_H */
/** @} */
