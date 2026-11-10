/**
 * @file    sine_gen.c
 * @brief   Implementacao do gerador senoidal DDS.
 * @ingroup SINE_GEN
 */
#include "sine_gen.h"
#include "app_config.h"
#include "bsp.h"
#include <math.h>

/** @brief Deslocamento que extrai o indice da tabela do acumulador de fase. */
#define PHASE_SHIFT (32U - SINE_GEN_LUT_BITS)

#define TWO_PI 6.28318530717958647692f /**< 2*pi */

static uint16_t          s_lut[SINE_GEN_LUT_SIZE]; /**< Seno em contagens de compare */
static volatile uint32_t s_phase_acc = 0U;         /**< Acumulador de fase (ISR)     */
static volatile uint32_t s_phase_inc = 0U;         /**< Incremento por amostra       */
static float             s_freq_hz   = 0.0f;       /**< Frequencia programada        */
static float             s_sample_hz = 0.0f;       /**< Taxa de amostragem (= f_pwm) */
static bool              s_running   = false;      /**< Saida ativa?                 */

/**
 * @brief Passo do DDS, chamado pelo BSP a cada periodo de PWM (ISR, 100 kHz).
 * @details O compare escrito aqui entra em vigor no proximo update (preload),
 *          portanto o duty nunca muda no meio de um periodo.
 */
static void sine_gen_on_pwm_update(void)
{
    s_phase_acc += s_phase_inc;
    bsp_pwm_set_compare(s_lut[s_phase_acc >> PHASE_SHIFT]);
}

void sine_gen_init(void)
{
    const float half = (float)bsp_pwm_get_period_counts() / 2.0f;

    /* compare[n] = (P/2) * (1 + m * sin(2*pi*n/N)), arredondado */
    for (uint32_t n = 0U; n < SINE_GEN_LUT_SIZE; n++)
    {
        float s = sinf(TWO_PI * (float)n / (float)SINE_GEN_LUT_SIZE);
        s_lut[n] = (uint16_t)(half * (1.0f + APP_SINE_MOD_INDEX * s) + 0.5f);
    }

    s_sample_hz = (float)bsp_pwm_get_carrier_hz();
    s_phase_acc = 0U;
    (void)sine_gen_set_frequency(APP_SINE_FREQ_DEFAULT_HZ);

    bsp_pwm_set_update_callback(sine_gen_on_pwm_update);
}

void sine_gen_start(void)
{
    bsp_pwm_set_compare(s_lut[s_phase_acc >> PHASE_SHIFT]);
    bsp_pwm_start();
    s_running = true;
}

void sine_gen_stop(void)
{
    bsp_pwm_stop();
    s_running = false;
}

bool sine_gen_is_running(void)
{
    return s_running;
}

bool sine_gen_set_frequency(float freq_hz)
{
    /* A comparacao negada tambem rejeita NaN */
    if (!((freq_hz >= APP_SINE_FREQ_MIN_HZ) && (freq_hz <= APP_SINE_FREQ_MAX_HZ)))
    {
        return false;
    }

    /* incremento = f * 2^32 / fs. Escrita de 32 bits e atomica no Cortex-M,
     * entao a ISR nunca ve um valor parcial. */
    s_phase_inc = (uint32_t)((double)freq_hz * 4294967296.0 / (double)s_sample_hz);
    s_freq_hz   = freq_hz;
    return true;
}

float sine_gen_get_frequency(void)
{
    return s_freq_hz;
}
