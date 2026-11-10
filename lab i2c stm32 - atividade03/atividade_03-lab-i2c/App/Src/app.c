/**
 * @file    app.c
 * @brief   Superloop da aplicacao: amostragem, filtro, telas e log.
 * @ingroup APP
 */
#include "app.h"
#include "app_config.h"
#include "attitude.h"
#include "ui.h"
#include "bsp.h"
#include <stdio.h>

/** @brief Estados da aplicacao. */
typedef enum
{
    APP_STATE_INIT = 0, /**< Aguardando SSD1306 e MPU6050 responderem */
    APP_STATE_RUN       /**< Amostrando e exibindo                    */
} app_state_t;

static app_state_t        s_state;          /**< Estado atual                        */
static attitude_t         s_att;            /**< Estimador de atitude                */
static bsp_mpu6050_data_t s_imu;            /**< Ultima amostra em unid. engenharia  */
static ui_screen_t        s_screen;         /**< Tela selecionada                    */
static bool               s_display_ok;     /**< SSD1306 inicializado                */
static bool               s_frame_busy;     /**< Quadro sendo enviado pagina a pagina */
static uint32_t           s_sample_cycles;  /**< Instante (DWT) da ultima amostra    */
static uint32_t           s_read_errors;    /**< Falhas consecutivas de leitura      */
static uint32_t           s_last_frame_ms;  /**< Inicio do ultimo quadro             */
static uint32_t           s_last_log_ms;    /**< Ultima linha de log                 */
static uint32_t           s_last_retry_ms;  /**< Ultima tentativa de inicializacao   */
static uint32_t           s_last_blink_ms;  /**< Ultima inversao do LED verde        */

/** @brief Mostra uma mensagem no display imediatamente (bloqueante). */
static void show_message(const char *l1, const char *l2, const char *l3)
{
    if (s_display_ok)
    {
        ui_draw_message(l1, l2, l3);
        (void)bsp_ssd1306_update();
        s_frame_busy = false;
    }
}

/**
 * @brief Calibra o bias do giroscopio e reinicia o filtro.
 * @return @c true se a calibracao foi concluida.
 */
static bool calibrate(void)
{
    printf("Calibrando giroscopio (%u amostras) - mantenha o sensor parado...\r\n",
           (unsigned)APP_GYRO_CALIB_SAMPLES);
    show_message("Calibrando giro...", "Mantenha o sensor", "PARADO");

    if (!bsp_mpu6050_calibrate_gyro(APP_GYRO_CALIB_SAMPLES))
    {
        printf("ERRO: falha de leitura durante a calibracao\r\n");
        return false;
    }

    attitude_init(&s_att, APP_FILTER_ALPHA);
    s_sample_cycles = bsp_timing_cycles();
    s_read_errors   = 0U;
    printf("Calibracao concluida. Yaw zerado.\r\n");
    return true;
}

/**
 * @brief Tenta inicializar display e sensor; em caso de sucesso calibra e
 *        passa para @ref APP_STATE_RUN.
 */
static void try_start(void)
{
    if (!s_display_ok)
    {
        s_display_ok = bsp_ssd1306_init();
        printf("SSD1306 (0x%02X): %s\r\n", (unsigned)BSP_SSD1306_ADDR,
               s_display_ok ? "ok" : "NAO ENCONTRADO");
        show_message("Lab I2C - Virtus-CC", "MPU6050 + SSD1306", "Iniciando...");
    }

    if (!bsp_mpu6050_init())
    {
        printf("MPU6050 (0x%02X): NAO ENCONTRADO - verifique a fiacao\r\n",
               (unsigned)BSP_MPU6050_ADDR);
        show_message("ERRO I2C", "MPU6050 nao", "encontrado");
        bsp_led_write(BSP_LED_RED, true);
        return;
    }
    printf("MPU6050 (0x%02X): ok, WHO_AM_I = 0x%02X\r\n",
           (unsigned)BSP_MPU6050_ADDR, (unsigned)bsp_mpu6050_who_am_i());

    if (!calibrate())
    {
        bsp_led_write(BSP_LED_RED, true);
        return;
    }

    bsp_led_write(BSP_LED_RED, false);
    s_state = APP_STATE_RUN;
}

/** @brief Le o MPU6050 e executa um passo do filtro quando o periodo vence. */
static void task_sample(void)
{
    uint32_t elapsed_us = bsp_timing_elapsed_us(s_sample_cycles);
    if (elapsed_us < APP_SAMPLE_PERIOD_US)
    {
        return;
    }
    s_sample_cycles = bsp_timing_cycles();

    if (bsp_mpu6050_read(&s_imu))
    {
        /* dt MEDIDO (nao o nominal): tolera o jitter causado pelo display */
        attitude_update(&s_att, s_imu.accel_g, s_imu.gyro_dps,
                        (float)elapsed_us * 1.0e-6f);
        s_read_errors = 0U;
        bsp_led_write(BSP_LED_RED, false);
    }
    else
    {
        bsp_led_write(BSP_LED_RED, true);
        if (++s_read_errors >= APP_MAX_READ_ERRORS)
        {
            printf("ERRO: MPU6050 parou de responder (%lu erros I2C no total)\r\n",
                   (unsigned long)bsp_i2c_error_count());
            s_display_ok = false;           /* reinicializa tudo */
            s_state      = APP_STATE_INIT;
        }
    }
}

/** @brief Trata os eventos do botao B1. */
static void task_button(void)
{
    switch (bsp_button_poll())
    {
    case BSP_BUTTON_EVT_SHORT:
        s_screen = (ui_screen_t)((s_screen + 1) % UI_SCREEN_COUNT);
        bsp_led_write(BSP_LED_BLUE, s_screen == UI_SCREEN_ANIM);
        break;

    case BSP_BUTTON_EVT_LONG:
        if (s_state == APP_STATE_RUN)
        {
            if (!calibrate())
            {
                s_state = APP_STATE_INIT;
            }
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Atualiza o display sem bloquear a amostragem: desenha um quadro no
 *        framebuffer e o envia uma pagina (~3 ms) por iteracao do superloop.
 */
static void task_display(void)
{
    if (!s_display_ok)
    {
        return;
    }

    if (!s_frame_busy)
    {
        uint32_t now = bsp_timing_millis();
        if ((now - s_last_frame_ms) < APP_DISPLAY_PERIOD_MS)
        {
            return;
        }
        s_last_frame_ms = now;

        if (s_screen == UI_SCREEN_ANIM)
        {
            ui_draw_anim(&s_att);
        }
        else
        {
            ui_draw_data(&s_imu, &s_att);
        }
        s_frame_busy = true;
    }

    if (bsp_ssd1306_update_step())
    {
        s_frame_busy = false;               /* quadro completo */
    }
}

/** @brief Envia uma linha de telemetria ao console serial. */
static void task_log(void)
{
    char a[3][12], g[3][12], e[3][12];
    uint32_t now = bsp_timing_millis();

    if ((now - s_last_log_ms) < APP_LOG_PERIOD_MS)
    {
        return;
    }
    s_last_log_ms = now;

    for (int i = 0; i < 3; i++)
    {
        ui_format_fixed(a[i], s_imu.accel_ms2[i], 2, 7);
        ui_format_fixed(g[i], s_imu.gyro_dps[i], 1, 7);
    }
    ui_format_fixed(e[0], s_att.roll, 1, 7);
    ui_format_fixed(e[1], s_att.pitch, 1, 7);
    ui_format_fixed(e[2], s_att.yaw, 1, 7);

    printf("a[m/s2]=%s %s %s | w[dps]=%s %s %s | roll=%s pitch=%s yaw=%s\r\n",
           a[0], a[1], a[2], g[0], g[1], g[2], e[0], e[1], e[2]);
}

void app_init(void)
{
    s_state      = APP_STATE_INIT;
    s_screen     = UI_SCREEN_DATA;
    s_display_ok = false;
    s_frame_busy = false;
    attitude_init(&s_att, APP_FILTER_ALPHA);

    printf("\r\n=== Lab I2C STM32 - MPU6050 + SSD1306 (Virtus-CC) ===\r\n");
    printf("Filtro complementar: alpha = 0.%02u, Ts = %u us\r\n",
           (unsigned)(APP_FILTER_ALPHA * 100.0f + 0.5f),
           (unsigned)APP_SAMPLE_PERIOD_US);
    printf("Botao B1: clique = troca de tela | segurar 1 s = recalibra e zera yaw\r\n");

    try_start();
    s_last_retry_ms = bsp_timing_millis();
}

void app_loop(void)
{
    uint32_t now = bsp_timing_millis();

    if ((now - s_last_blink_ms) >= 500U)    /* heartbeat */
    {
        s_last_blink_ms = now;
        bsp_led_toggle(BSP_LED_GREEN);
    }

    if (s_state == APP_STATE_INIT)
    {
        if ((now - s_last_retry_ms) >= APP_RETRY_PERIOD_MS)
        {
            try_start();
            s_last_retry_ms = bsp_timing_millis();
        }
        return;
    }

    task_sample();
    task_button();
    task_display();
    task_log();
}
