/**
 * @file    app.c
 * @brief   Superloop da aplicacao do digipot.
 * @ingroup APP
 */
#include "app.h"
#include "app_config.h"
#include "cli.h"
#include "digipot.h"
#include "ohm_ctrl.h"
#include "bsp.h"
#include <stdio.h>

static uint32_t s_last_blink_ms;    /**< Ultima inversao do LED verde */

void app_init(bool bsp_ok)
{
    printf("\r\n=== Lab ADC-DAC-DMA STM32 - Digipot LED + LDR (Virtus-CC) ===\r\n");
    printf("DAC_OUT1 (PA4) -> LED | LDR + %lu ohm -> ADC1_IN3 (PA3) | UART RX por DMA\r\n",
           (unsigned long)APP_R_SERIES_OHM);
    if (!bsp_ok)
    {
        printf("ERRO: falha ao iniciar ADC/DAC\r\n");
        bsp_led_write(BSP_LED_RED, true);
    }

    digipot_init();
    printf("Digite 'help' para a lista de comandos.\r\n");
    cli_init();
    s_last_blink_ms = bsp_millis();
}

void app_loop(void)
{
    uint32_t now = bsp_millis();

    if ((now - s_last_blink_ms) >= 500U)    /* heartbeat */
    {
        s_last_blink_ms = now;
        bsp_led_toggle(BSP_LED_GREEN);
    }

    cli_task();
    ohm_ctrl_task();

    /* Azul: com a malha ligada indica "travada"; senao, LUT calibrada. */
    bsp_led_write(BSP_LED_BLUE, ohm_ctrl_active() ? ohm_ctrl_locked()
                                                  : digipot_is_calibrated());
}
