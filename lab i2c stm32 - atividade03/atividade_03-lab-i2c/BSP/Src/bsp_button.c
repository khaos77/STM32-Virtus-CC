/**
 * @file    bsp_button.c
 * @brief   Debounce e deteccao de clique curto/longo do botao B1.
 * @ingroup BSP_BUTTON
 */
#include "bsp_button.h"
#include "main.h"
#include <stdbool.h>

static bool     s_stable;       /**< Estado ja filtrado (true = pressionado)   */
static bool     s_last_raw;     /**< Ultima leitura crua                       */
static uint32_t s_change_ms;    /**< Instante da ultima mudanca da leitura crua */
static uint32_t s_press_ms;     /**< Instante em que o botao foi pressionado   */
static bool     s_long_fired;   /**< Evento longo ja entregue neste aperto     */

void bsp_button_init(void)
{
    s_stable     = false;
    s_last_raw   = false;
    s_change_ms  = HAL_GetTick();
    s_press_ms   = 0U;
    s_long_fired = false;
}

bsp_button_evt_t bsp_button_poll(void)
{
    uint32_t now = HAL_GetTick();
    bool raw = (HAL_GPIO_ReadPin(BTN_USER_GPIO_Port, BTN_USER_Pin) == GPIO_PIN_SET);

    if (raw != s_last_raw)
    {
        s_last_raw  = raw;
        s_change_ms = now;          /* ainda oscilando: reinicia a janela */
    }

    if ((raw != s_stable) && ((now - s_change_ms) >= BSP_BUTTON_DEBOUNCE_MS))
    {
        s_stable = raw;
        if (s_stable)
        {
            s_press_ms   = now;
            s_long_fired = false;
        }
        else if (!s_long_fired)
        {
            return BSP_BUTTON_EVT_SHORT;
        }
    }

    if (s_stable && !s_long_fired && ((now - s_press_ms) >= BSP_BUTTON_LONG_MS))
    {
        s_long_fired = true;
        return BSP_BUTTON_EVT_LONG;
    }

    return BSP_BUTTON_EVT_NONE;
}
