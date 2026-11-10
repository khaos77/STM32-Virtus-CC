/**
 * @file    bsp_adc.c
 * @brief   ADC1 em modo continuo com DMA circular e media das amostras.
 * @ingroup BSP_ADC
 */
#include "bsp_adc.h"

static ADC_HandleTypeDef *s_hadc = NULL;              /**< ADC do divisor (ou NULL)      */
static volatile uint16_t  s_buf[BSP_ADC_BUF_LEN];     /**< Buffer escrito pelo DMA       */

/**
 * @brief (Re)inicia a conversao continua com DMA circular.
 * @return @c true em caso de sucesso.
 */
static bool adc_start(void)
{
    if (HAL_ADC_Start_DMA(s_hadc, (uint32_t *)s_buf, BSP_ADC_BUF_LEN) != HAL_OK)
    {
        return false;
    }
    /* O buffer e lido por consulta: nao ha motivo para ~860 interrupcoes/s. */
    __HAL_DMA_DISABLE_IT(s_hadc->DMA_Handle, DMA_IT_HT | DMA_IT_TC);
    return true;
}

bool bsp_adc_init(ADC_HandleTypeDef *hadc)
{
    s_hadc = hadc;
    if (s_hadc == NULL)
    {
        return false;
    }
    return adc_start();
}

uint16_t bsp_adc_read_raw(void)
{
    uint32_t sum = 0U;

    if (s_hadc == NULL)
    {
        return 0U;
    }

    /* Overrun: o ADC para de pedir DMA ate ser reiniciado. */
    if (__HAL_ADC_GET_FLAG(s_hadc, ADC_FLAG_OVR) != 0U)
    {
        (void)HAL_ADC_Stop_DMA(s_hadc);
        __HAL_ADC_CLEAR_FLAG(s_hadc, ADC_FLAG_OVR);
        (void)adc_start();
    }

    for (uint32_t i = 0U; i < BSP_ADC_BUF_LEN; i++)
    {
        sum += s_buf[i];            /* leitura de 16 bits e atomica no Cortex-M */
    }
    return (uint16_t)((sum + (BSP_ADC_BUF_LEN / 2U)) / BSP_ADC_BUF_LEN);
}

uint32_t bsp_adc_raw_to_mv(uint16_t raw)
{
    return (((uint32_t)raw * BSP_ADC_VREF_MV) + (BSP_ADC_FULL_SCALE / 2U))
           / BSP_ADC_FULL_SCALE;
}
