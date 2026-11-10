/**
 * @file    bsp_i2c.c
 * @brief   Implementacao do acesso ao barramento I2C sobre a HAL.
 * @ingroup BSP_I2C
 */
#include "bsp_i2c.h"

static I2C_HandleTypeDef *s_hi2c = NULL; /**< Barramento compartilhado        */
static uint32_t           s_errors;      /**< Contador de transacoes com erro */

/** @brief Converte o status da HAL em booleano e contabiliza falhas. */
static bool check(HAL_StatusTypeDef st)
{
    if (st != HAL_OK)
    {
        s_errors++;
        return false;
    }
    return true;
}

void bsp_i2c_init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c   = hi2c;
    s_errors = 0U;
}

bool bsp_i2c_is_ready(uint8_t addr7)
{
    if (s_hi2c == NULL)
    {
        return false;
    }
    return HAL_I2C_IsDeviceReady(s_hi2c, (uint16_t)(addr7 << 1), 2U,
                                 BSP_I2C_TIMEOUT_MS) == HAL_OK;
}

bool bsp_i2c_write_reg(uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (s_hi2c == NULL)
    {
        return false;
    }
    return check(HAL_I2C_Mem_Write(s_hi2c, (uint16_t)(addr7 << 1), reg,
                                   I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len,
                                   BSP_I2C_TIMEOUT_MS));
}

bool bsp_i2c_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (s_hi2c == NULL)
    {
        return false;
    }
    return check(HAL_I2C_Mem_Read(s_hi2c, (uint16_t)(addr7 << 1), reg,
                                  I2C_MEMADD_SIZE_8BIT, data, len,
                                  BSP_I2C_TIMEOUT_MS));
}

uint32_t bsp_i2c_error_count(void)
{
    return s_errors;
}
