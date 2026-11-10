/**
 * @file    bsp_dac.c
 * @brief   Acionamento do LED do digipot pelo DAC (canal 1).
 * @ingroup BSP_DAC
 */
#include "bsp_dac.h"

static DAC_HandleTypeDef *s_hdac = NULL;   /**< DAC do LED (ou NULL)      */
static uint16_t           s_code = 0U;     /**< Ultimo codigo escrito     */

bool bsp_dac_init(DAC_HandleTypeDef *hdac)
{
    s_hdac = hdac;
    s_code = 0U;
    if (s_hdac == NULL)
    {
        return false;
    }
    (void)HAL_DAC_SetValue(s_hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0U);
    return HAL_DAC_Start(s_hdac, DAC_CHANNEL_1) == HAL_OK;
}

void bsp_dac_write(uint16_t code)
{
    if (code > BSP_DAC_MAX_CODE)
    {
        code = BSP_DAC_MAX_CODE;
    }
    s_code = code;
    if (s_hdac != NULL)
    {
        (void)HAL_DAC_SetValue(s_hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, code);
    }
}

uint16_t bsp_dac_read(void)
{
    return s_code;
}

uint32_t bsp_dac_code_to_mv(uint16_t code)
{
    return (((uint32_t)code * 3300U) + (BSP_DAC_MAX_CODE / 2U)) / BSP_DAC_MAX_CODE;
}
