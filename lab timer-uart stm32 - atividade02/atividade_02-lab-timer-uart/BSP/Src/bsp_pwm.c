/**
 * @file    bsp_pwm.c
 * @brief   Implementacao da saida PWM sobre o TIM3_CH1.
 * @ingroup BSP_PWM
 */
#include "bsp_pwm.h"

#define BSP_PWM_CHANNEL TIM_CHANNEL_1 /**< Canal do timer ligado a PA6 */

static TIM_HandleTypeDef  *s_htim = NULL;       /**< Timer de PWM (ou NULL)      */
static bsp_pwm_update_cb_t s_update_cb = NULL;  /**< Callback de update (ou NULL) */

void bsp_pwm_init(TIM_HandleTypeDef *htim)
{
    s_htim = htim;
}

void bsp_pwm_set_update_callback(bsp_pwm_update_cb_t cb)
{
    s_update_cb = cb;
}

void bsp_pwm_start(void)
{
    if (s_htim != NULL)
    {
        HAL_TIM_PWM_Start(s_htim, BSP_PWM_CHANNEL);
        __HAL_TIM_ENABLE_IT(s_htim, TIM_IT_UPDATE);
    }
}

void bsp_pwm_stop(void)
{
    if (s_htim != NULL)
    {
        __HAL_TIM_DISABLE_IT(s_htim, TIM_IT_UPDATE);
        HAL_TIM_PWM_Stop(s_htim, BSP_PWM_CHANNEL);
    }
}

uint32_t bsp_pwm_get_period_counts(void)
{
    return (s_htim != NULL) ? (__HAL_TIM_GET_AUTORELOAD(s_htim) + 1U) : 0U;
}

uint32_t bsp_pwm_get_carrier_hz(void)
{
    if (s_htim == NULL)
    {
        return 0U;
    }

    /* TIM3 esta no APB1. Se o prescaler do APB1 for != 1, o clock dos timers
     * e o dobro do PCLK1 (RM0410, arvore de clock). */
    uint32_t tim_clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        tim_clk *= 2U;
    }

    return tim_clk / (s_htim->Instance->PSC + 1U) / bsp_pwm_get_period_counts();
}

void bsp_pwm_set_compare(uint32_t compare)
{
    __HAL_TIM_SET_COMPARE(s_htim, BSP_PWM_CHANNEL, compare);
}

/**
 * @brief Callback da HAL para o evento de update (chamado por
 *        @c HAL_TIM_IRQHandler em @c TIM3_IRQHandler).
 * @param htim Timer que gerou o evento.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim == s_htim) && (s_update_cb != NULL))
    {
        s_update_cb();
    }
}
