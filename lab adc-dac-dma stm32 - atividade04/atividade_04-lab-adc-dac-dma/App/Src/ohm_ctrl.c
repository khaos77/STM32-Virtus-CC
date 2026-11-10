/**
 * @file    ohm_ctrl.c
 * @brief   Controlador integral que mantem o digipot em um valor em ohms.
 * @ingroup OHM_CTRL
 */
#include "ohm_ctrl.h"
#include "app_config.h"
#include "digipot.h"
#include "bsp.h"
#include <stdio.h>

/** @brief Passo do DAC quando a medida esta fora da faixa util do ADC. */
#define OUT_OF_RANGE_STEP 40

static bool     s_active = false;   /**< Malha ligada                          */
static bool     s_locked = false;   /**< Erro dentro da tolerancia             */
static uint32_t s_target = 0U;      /**< Alvo em ohms                          */
static int32_t  s_dac    = 0;       /**< Estado do integrador (codigo do DAC)  */
static uint32_t s_last_step_ms;     /**< Ultimo passo do controlador           */
static uint32_t s_last_report_ms;   /**< Ultimo relatorio no console           */

void ohm_ctrl_start(uint32_t r_ohm)
{
    if (r_ohm == 0U)
    {
        return;
    }
    s_target = r_ohm;
    s_locked = false;

    /* Feedforward: parte do ponto previsto pela LUT, se ela estiver calibrada. */
    if (digipot_is_calibrated())
    {
        digipot_set(digipot_res_for_ohms(r_ohm));
    }
    s_dac            = (int32_t)digipot_get_dac();
    s_last_step_ms   = bsp_millis();
    s_last_report_ms = s_last_step_ms;
    s_active         = true;
}

void ohm_ctrl_stop(void)
{
    s_active = false;
    s_locked = false;
}

bool ohm_ctrl_active(void)
{
    return s_active;
}

bool ohm_ctrl_locked(void)
{
    return s_active && s_locked;
}

void ohm_ctrl_task(void)
{
    digipot_meas_t m;
    int32_t        err_pm;  /* erro relativo em partes por mil */
    int32_t        step;
    uint32_t       now = bsp_millis();

    if (!s_active || ((now - s_last_step_ms) < APP_OHM_PERIOD_MS))
    {
        return;
    }
    s_last_step_ms = now;

    digipot_measure_fast(&m);

    if (!m.valid)
    {
        /* ADC baixo = LDR escuro = R alta demais -> mais luz (e vice-versa). */
        step     = (m.adc_raw < APP_ADC_MIN_VALID) ? OUT_OF_RANGE_STEP : -OUT_OF_RANGE_STEP;
        err_pm   = (step > 0) ? 1000 : -1000;
        s_locked = false;
    }
    else
    {
        int64_t diff = (int64_t)m.r_ohm - (int64_t)s_target;
        int64_t e    = (diff * 1000) / (int64_t)s_target;

        if (e > 1000)  { e = 1000; }
        if (e < -1000) { e = -1000; }
        err_pm = (int32_t)e;

        step = (err_pm * APP_OHM_GAIN_CODES) / 1000;
        if ((step == 0) && (diff != 0))
        {
            step = (diff > 0) ? 1 : -1;     /* nunca fica preso por truncamento */
        }
        s_locked = (err_pm <= APP_OHM_LOCK_PERMILLE) && (err_pm >= -APP_OHM_LOCK_PERMILLE);
        if (s_locked && (step > 1 || step < -1))
        {
            step = (step > 0) ? 1 : -1;     /* ajuste fino quando travada */
        }
    }

    s_dac += step;
    if (s_dac < 0)                        { s_dac = 0; }
    if (s_dac > (int32_t)BSP_DAC_MAX_CODE) { s_dac = (int32_t)BSP_DAC_MAX_CODE; }
    digipot_set_dac((uint16_t)s_dac);

    if ((now - s_last_report_ms) >= APP_OHM_REPORT_MS)
    {
        s_last_report_ms = now;
        int32_t abs_pm = (err_pm < 0) ? -err_pm : err_pm;

        printf("ohm: alvo=%lu medido=%lu erro=%s%ld.%ld%% dac=%ld %s\r\n",
               (unsigned long)s_target, (unsigned long)m.r_ohm,
               (err_pm < 0) ? "-" : "+", (long)(abs_pm / 10), (long)(abs_pm % 10),
               (long)s_dac,
               !m.valid ? "(fora da faixa util)" : (s_locked ? "(travado)" : ""));
    }
}
