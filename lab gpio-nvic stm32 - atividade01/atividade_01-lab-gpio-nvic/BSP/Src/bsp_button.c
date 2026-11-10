/**
 * @file    bsp_button.c
 * @brief   Implementacao do botao com debounce por tempo.
 * @ingroup BSP_BUTTON
 */
#include "bsp_button.h"

static volatile bool     s_event   = false; /**< Evento pendente para o laco principal */
static volatile uint32_t s_last_ms = 0u;    /**< Instante do ultimo evento aceito */

void bsp_button_init(void)
{
    s_event   = false;
    s_last_ms = HAL_GetTick();
}

bool bsp_button_take_event(void)
{
    if (s_event)
    {
        s_event = false;
        return true;
    }
    return false;
}

bool bsp_button_is_pressed(void)
{
    /* Na NUCLEO-144 o B1 tem pull-down externo: pressionado = nivel alto. */
    return HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_SET;
}

void bsp_button_irq(void)
{
    const uint32_t now = HAL_GetTick();

    if ((now - s_last_ms) >= BSP_BUTTON_DEBOUNCE_MS)
    {
        s_last_ms = now;
        s_event   = true;
    }
}
