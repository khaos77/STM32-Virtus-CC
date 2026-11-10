/**
 * @file    app.c
 * @brief   Superloop da aplicacao.
 * @ingroup APP
 */
#include "app.h"
#include "app_config.h"
#include "bsp.h"
#include "cli.h"
#include "sine_gen.h"

static uint32_t s_last_blink = 0U; /**< Instante do ultimo pisca do LED de status */

void app_init(void)
{
    sine_gen_init();
    sine_gen_start();
    cli_init();
}

void app_loop(void)
{
    cli_process();

    if ((bsp_millis() - s_last_blink) >= APP_HEARTBEAT_MS)
    {
        s_last_blink = bsp_millis();
        bsp_led_toggle(BSP_LED_STATUS);
    }
}
