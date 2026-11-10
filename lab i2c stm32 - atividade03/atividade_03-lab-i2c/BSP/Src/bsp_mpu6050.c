/**
 * @file    bsp_mpu6050.c
 * @brief   Driver do MPU6050: inicializacao, leitura e conversao de unidades.
 * @ingroup BSP_MPU6050
 */
#include "bsp_mpu6050.h"
#include "bsp_i2c.h"
#include "bsp_timing.h"

/** @name Mapa de registradores utilizados
 *  @{ */
#define REG_SMPLRT_DIV    0x19U /**< Divisor da taxa de amostragem          */
#define REG_CONFIG        0x1AU /**< Filtro passa-baixas digital (DLPF)     */
#define REG_GYRO_CONFIG   0x1BU /**< Fundo de escala do giroscopio          */
#define REG_ACCEL_CONFIG  0x1CU /**< Fundo de escala do acelerometro        */
#define REG_ACCEL_XOUT_H  0x3BU /**< Primeiro dos 14 registradores de dados */
#define REG_PWR_MGMT_1    0x6BU /**< Reset, sleep e fonte de clock          */
#define REG_WHO_AM_I      0x75U /**< Identificacao do dispositivo           */
/** @} */

#define PWR_DEVICE_RESET  0x80U /**< PWR_MGMT_1: reinicia o chip           */
#define PWR_CLK_PLL_XGYRO 0x01U /**< PWR_MGMT_1: acordado, clock = PLL X   */
#define CFG_DLPF_44HZ     0x03U /**< CONFIG: DLPF acel 44 Hz / giro 42 Hz  */
#define SMPLRT_200HZ      0x04U /**< SMPLRT_DIV: 1 kHz / (1 + 4)           */
#define GYRO_FS_500DPS    0x08U /**< GYRO_CONFIG: FS_SEL = 1               */
#define ACCEL_FS_4G       0x08U /**< ACCEL_CONFIG: AFS_SEL = 1             */

/** @brief Bias do giroscopio em graus/s, estimado na calibracao. */
static float s_gyro_bias_dps[3];

/** @brief Escreve um unico registrador. */
static bool write_reg(uint8_t reg, uint8_t value)
{
    return bsp_i2c_write_reg(BSP_MPU6050_ADDR, reg, &value, 1U);
}

/** @brief Junta dois bytes big-endian em um inteiro de 16 bits com sinal. */
static int16_t be16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

bool bsp_mpu6050_init(void)
{
    s_gyro_bias_dps[0] = 0.0f;
    s_gyro_bias_dps[1] = 0.0f;
    s_gyro_bias_dps[2] = 0.0f;

    if (!bsp_i2c_is_ready(BSP_MPU6050_ADDR))
    {
        return false;
    }

    if (!write_reg(REG_PWR_MGMT_1, PWR_DEVICE_RESET))
    {
        return false;
    }
    bsp_timing_delay_ms(100U);              /* tempo de reset (datasheet) */

    bool ok = write_reg(REG_PWR_MGMT_1, PWR_CLK_PLL_XGYRO);
    bsp_timing_delay_ms(10U);               /* estabilizacao do PLL */
    ok = ok && write_reg(REG_CONFIG, CFG_DLPF_44HZ);
    ok = ok && write_reg(REG_SMPLRT_DIV, SMPLRT_200HZ);
    ok = ok && write_reg(REG_GYRO_CONFIG, GYRO_FS_500DPS);
    ok = ok && write_reg(REG_ACCEL_CONFIG, ACCEL_FS_4G);
    bsp_timing_delay_ms(50U);               /* primeiras amostras validas */
    return ok;
}

uint8_t bsp_mpu6050_who_am_i(void)
{
    uint8_t id = 0U;
    if (!bsp_i2c_read_reg(BSP_MPU6050_ADDR, REG_WHO_AM_I, &id, 1U))
    {
        return 0U;
    }
    return id;
}

bool bsp_mpu6050_read_raw(bsp_mpu6050_raw_t *raw)
{
    uint8_t b[14];

    if (!bsp_i2c_read_reg(BSP_MPU6050_ADDR, REG_ACCEL_XOUT_H, b, sizeof(b)))
    {
        return false;
    }

    raw->accel[0] = be16(&b[0]);
    raw->accel[1] = be16(&b[2]);
    raw->accel[2] = be16(&b[4]);
    raw->temp     = be16(&b[6]);
    raw->gyro[0]  = be16(&b[8]);
    raw->gyro[1]  = be16(&b[10]);
    raw->gyro[2]  = be16(&b[12]);
    return true;
}

void bsp_mpu6050_convert(const bsp_mpu6050_raw_t *raw, bsp_mpu6050_data_t *out)
{
    for (int i = 0; i < 3; i++)
    {
        out->accel_g[i]   = (float)raw->accel[i] / BSP_MPU6050_ACCEL_LSB_PER_G;
        out->accel_ms2[i] = out->accel_g[i] * BSP_MPU6050_GRAVITY_MS2;
        out->gyro_dps[i]  = ((float)raw->gyro[i] / BSP_MPU6050_GYRO_LSB_PER_DPS)
                            - s_gyro_bias_dps[i];
    }
    out->temp_c = ((float)raw->temp / 340.0f) + 36.53f;
}

bool bsp_mpu6050_read(bsp_mpu6050_data_t *out)
{
    bsp_mpu6050_raw_t raw;

    if (!bsp_mpu6050_read_raw(&raw))
    {
        return false;
    }
    bsp_mpu6050_convert(&raw, out);
    return true;
}

bool bsp_mpu6050_calibrate_gyro(uint16_t samples)
{
    int32_t sum[3] = { 0, 0, 0 };
    bsp_mpu6050_raw_t raw;

    if (samples == 0U)
    {
        return false;
    }

    for (uint16_t n = 0U; n < samples; n++)
    {
        if (!bsp_mpu6050_read_raw(&raw))
        {
            return false;                   /* mantem o bias anterior */
        }
        sum[0] += raw.gyro[0];
        sum[1] += raw.gyro[1];
        sum[2] += raw.gyro[2];
        bsp_timing_delay_ms(3U);
    }

    for (int i = 0; i < 3; i++)
    {
        s_gyro_bias_dps[i] = ((float)sum[i] / (float)samples)
                             / BSP_MPU6050_GYRO_LSB_PER_DPS;
    }
    return true;
}
