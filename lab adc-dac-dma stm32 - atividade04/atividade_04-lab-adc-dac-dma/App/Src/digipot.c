/**
 * @file    digipot.c
 * @brief   Digipot LED + LDR: LUT, medida da resistencia e calibracao.
 * @ingroup DIGIPOT
 */
#include "digipot.h"
#include "app_config.h"
#include "bsp.h"
#include <string.h>

/** @brief Minimo de pontos validos e monotonicos exigido pela autocalibracao. */
#define CAL_MIN_POINTS 8U

static uint16_t s_lut[DIGIPOT_LUT_SIZE]; /**< LUT res -> codigo do DAC (RAM)        */
static uint32_t s_r_min = 0U;            /**< Resistencia nominal de res = 0        */
static uint32_t s_r_max = 0U;            /**< Resistencia nominal de res = 255      */
static uint8_t  s_res   = 0U;            /**< Ultima palavra digital aplicada       */

/** @brief Le um @c uint16 little-endian. */
static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/** @brief Le um @c uint32 little-endian. */
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/** @brief Preenche uma medida a partir do codigo medio do ADC. */
static void fill_meas(digipot_meas_t *m, uint16_t raw)
{
    m->adc_raw = raw;
    m->mv      = bsp_adc_raw_to_mv(raw);
    m->r_ohm   = digipot_adc_to_ohms(raw);
    m->valid   = (raw >= APP_ADC_MIN_VALID) && (raw <= APP_ADC_MAX_VALID);
}

/** @brief Resistencia nominal linear entre @p r_min e @p r_max. */
static uint32_t nominal(uint8_t res, uint32_t r_min, uint32_t r_max)
{
    uint64_t span = (uint64_t)(r_max - r_min);
    return r_min + (uint32_t)((span * res + 127U) / 255U);
}

void digipot_init(void)
{
    digipot_lut_default();
    digipot_set(0U);
}

void digipot_set(uint8_t res)
{
    s_res = res;
    bsp_dac_write(s_lut[res]);
}

uint8_t digipot_get(void)
{
    return s_res;
}

void digipot_set_dac(uint16_t code)
{
    bsp_dac_write(code);
}

uint16_t digipot_get_dac(void)
{
    return bsp_dac_read();
}

void digipot_measure(digipot_meas_t *m)
{
    uint32_t sum = 0U;

    for (uint32_t i = 0U; i < APP_MEAS_WINDOWS; i++)
    {
        bsp_delay_ms(3U);           /* > 2,3 ms: buffer do DMA totalmente novo */
        sum += bsp_adc_read_raw();
    }
    fill_meas(m, (uint16_t)((sum + (APP_MEAS_WINDOWS / 2U)) / APP_MEAS_WINDOWS));
}

void digipot_measure_fast(digipot_meas_t *m)
{
    fill_meas(m, bsp_adc_read_raw());
}

uint32_t digipot_adc_to_ohms(uint16_t raw)
{
    if (raw == 0U)
    {
        return DIGIPOT_R_OPEN;
    }
    if (raw >= BSP_ADC_FULL_SCALE)
    {
        return 0U;
    }
    /* R_LDR = R_S * (4095 - N) / N, com arredondamento */
    return (uint32_t)((((uint64_t)APP_R_SERIES_OHM * (BSP_ADC_FULL_SCALE - raw))
                       + (raw / 2U)) / raw);
}

bool digipot_is_calibrated(void)
{
    return s_r_max > s_r_min;
}

uint32_t digipot_nominal_ohms(uint8_t res)
{
    if (!digipot_is_calibrated())
    {
        return 0U;
    }
    return nominal(res, s_r_min, s_r_max);
}

uint8_t digipot_res_for_ohms(uint32_t r_ohm)
{
    if (!digipot_is_calibrated() || (r_ohm <= s_r_min))
    {
        return 0U;
    }
    if (r_ohm >= s_r_max)
    {
        return 255U;
    }
    uint64_t span = (uint64_t)(s_r_max - s_r_min);
    return (uint8_t)((((uint64_t)(r_ohm - s_r_min) * 255U) + (span / 2U)) / span);
}

void digipot_range(uint32_t *r_min, uint32_t *r_max)
{
    *r_min = s_r_min;
    *r_max = s_r_max;
}

const uint16_t *digipot_lut(void)
{
    return s_lut;
}

void digipot_lut_default(void)
{
    const uint32_t span = BSP_DAC_MAX_CODE - APP_DAC_LED_MIN_CODE;

    for (uint32_t res = 0U; res < DIGIPOT_LUT_SIZE; res++)
    {
        /* res = 0 -> LED no maximo (menor R); res = 255 -> limiar do LED */
        s_lut[res] = (uint16_t)(BSP_DAC_MAX_CODE - ((res * span + 127U) / 255U));
    }
    s_r_min = 0U;
    s_r_max = 0U;
    digipot_set(s_res);
}

digipot_status_t digipot_lut_load(const uint8_t *data)
{
    uint16_t crc_rx   = rd_u16(&data[DIGIPOT_FRAME_BODY_SIZE]);
    uint32_t r_min    = rd_u32(&data[0]);
    uint32_t r_max    = rd_u32(&data[4]);
    const uint8_t *lut = &data[8];

    if (digipot_crc16(data, DIGIPOT_FRAME_BODY_SIZE) != crc_rx)
    {
        return DIGIPOT_ERR_CRC;
    }
    if (r_min >= r_max)
    {
        return DIGIPOT_ERR_VALUE;
    }
    for (uint32_t i = 0U; i < DIGIPOT_LUT_SIZE; i++)
    {
        if (rd_u16(&lut[2U * i]) > BSP_DAC_MAX_CODE)
        {
            return DIGIPOT_ERR_VALUE;
        }
    }

    /* Quadro integro: so agora a LUT em uso e substituida. */
    for (uint32_t i = 0U; i < DIGIPOT_LUT_SIZE; i++)
    {
        s_lut[i] = rd_u16(&lut[2U * i]);
    }
    s_r_min = r_min;
    s_r_max = r_max;
    digipot_set(s_res);
    return DIGIPOT_OK;
}

void digipot_build_lut(const uint16_t *dac, const uint32_t *r, uint32_t n,
                       uint32_t r_min, uint32_t r_max)
{
    uint32_t j = n - 1U;            /* segmento [j-1, j] que contem o alvo */

    /* res cresce -> alvo cresce -> anda para o inicio da curva (R maior). */
    for (uint32_t res = 0U; res < DIGIPOT_LUT_SIZE; res++)
    {
        uint32_t target = nominal((uint8_t)res, r_min, r_max);
        uint16_t code;

        while ((j > 1U) && (r[j - 1U] < target))
        {
            j--;
        }

        if (target <= r[n - 1U])
        {
            code = dac[n - 1U];
        }
        else if (target >= r[0])
        {
            code = dac[0];
        }
        else
        {
            /* r[j-1] >= target > r[j]: interpola o codigo na condutancia
             * (G = 1/R), que e quase linear com a corrente do LED:
             * frac = (1/target - 1/r0) / (1/r1 - 1/r0)                     */
            double r0   = (double)r[j - 1U];
            double r1   = (double)r[j];
            double frac = (r1 * (r0 - (double)target)) / ((double)target * (r0 - r1));
            code = (uint16_t)(dac[j - 1U] +
                              (uint16_t)(((double)(dac[j] - dac[j - 1U]) * frac) + 0.5));
        }
        s_lut[res] = code;
    }
    s_r_min = r_min;
    s_r_max = r_max;
    digipot_set(s_res);
}

digipot_status_t digipot_autocal(uint32_t r_min_req, uint32_t r_max_req)
{
    static uint16_t dac[APP_CAL_POINTS];
    static uint32_t r[APP_CAL_POINTS];
    const uint32_t  span  = BSP_DAC_MAX_CODE - APP_DAC_LED_MIN_CODE;
    const uint16_t  saved = bsp_dac_read();
    uint32_t        n     = 0U;
    uint32_t        r_min;
    uint32_t        r_max;

    /* Luz crescente: o LDR responde mais rapido ao clarear que ao escurecer. */
    bsp_dac_write(APP_DAC_LED_MIN_CODE);
    bsp_delay_ms(APP_SETTLE_FIRST_MS);

    for (uint32_t i = 0U; i < APP_CAL_POINTS; i++)
    {
        digipot_meas_t m;
        uint16_t code = (uint16_t)(APP_DAC_LED_MIN_CODE +
                                   ((i * span) / (APP_CAL_POINTS - 1U)));
        bsp_dac_write(code);
        bsp_delay_ms(APP_SETTLE_MS);
        digipot_measure(&m);

        /* Guarda so pontos dentro da faixa util e estritamente decrescentes. */
        if (m.valid && ((n == 0U) || (m.r_ohm < r[n - 1U])))
        {
            dac[n] = code;
            r[n]   = m.r_ohm;
            n++;
        }
    }

    if (n < CAL_MIN_POINTS)
    {
        bsp_dac_write(saved);
        return DIGIPOT_ERR_CAL_POINTS;
    }

    r_max = r[0];
    r_min = r[n - 1U];
    if ((r_max_req != 0U) && (r_max_req < r_max))
    {
        r_max = r_max_req;
    }
    if ((r_min_req != 0U) && (r_min_req > r_min))
    {
        r_min = r_min_req;
    }
    if (r_min >= r_max)
    {
        bsp_dac_write(saved);
        return DIGIPOT_ERR_VALUE;
    }

    digipot_build_lut(dac, r, n, r_min, r_max);
    return DIGIPOT_OK;
}

uint16_t digipot_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint32_t i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; b++)
        {
            crc = ((crc & 0x8000U) != 0U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                                          : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

const char *digipot_status_str(digipot_status_t st)
{
    switch (st)
    {
    case DIGIPOT_OK:             return "ok";
    case DIGIPOT_ERR_CRC:        return "CRC invalido";
    case DIGIPOT_ERR_VALUE:      return "valor fora da faixa (DAC > 4095 ou r_min >= r_max)";
    case DIGIPOT_ERR_CAL_POINTS: return "poucos pontos validos (verifique LED, LDR e resistor serie)";
    default:                     return "erro desconhecido";
    }
}
