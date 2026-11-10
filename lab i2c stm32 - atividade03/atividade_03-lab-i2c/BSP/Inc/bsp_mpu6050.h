/**
 * @file    bsp_mpu6050.h
 * @brief   Biblioteca do MPU6050 (acelerometro + giroscopio triaxiais, I2C).
 *
 * @details Configuracao aplicada por @ref bsp_mpu6050_init :
 *
 * | Registrador        | Valor | Efeito                                         |
 * |--------------------|-------|------------------------------------------------|
 * | PWR_MGMT_1 (0x6B)  | 0x80  | reset do dispositivo                           |
 * | PWR_MGMT_1 (0x6B)  | 0x01  | sai do sleep, clock = PLL do giroscopio X      |
 * | CONFIG (0x1A)      | 0x03  | filtro passa-baixas digital (DLPF) ~44 Hz      |
 * | SMPLRT_DIV (0x19)  | 0x04  | taxa de amostragem = 1 kHz / (1 + 4) = 200 Hz  |
 * | GYRO_CONFIG (0x1B) | 0x08  | fundo de escala +-500 graus/s                  |
 * | ACCEL_CONFIG (0x1C)| 0x08  | fundo de escala +-4 g                          |
 *
 * Conversao para unidades de engenharia:
 *
 * @f[ a\,[g] = \frac{raw_a}{8192}, \qquad a\,[m/s^2] = a\,[g]\cdot 9{,}80665 @f]
 * @f[ \omega\,[^\circ/s] = \frac{raw_\omega}{65{,}5} - bias_\omega @f]
 * @f[ T\,[^\circ C] = \frac{raw_T}{340} + 36{,}53 @f]
 *
 * @defgroup BSP_MPU6050 MPU6050
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_MPU6050_H
#define BSP_MPU6050_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Endereco I2C de 7 bits (pino AD0 em GND). Com AD0 = VCC use 0x69. */
#define BSP_MPU6050_ADDR             0x68U

/** @brief Sensibilidade do acelerometro em +-4 g (LSB/g). */
#define BSP_MPU6050_ACCEL_LSB_PER_G  8192.0f
/** @brief Sensibilidade do giroscopio em +-500 graus/s (LSB/(grau/s)). */
#define BSP_MPU6050_GYRO_LSB_PER_DPS 65.5f
/** @brief Aceleracao da gravidade padrao (m/s^2). */
#define BSP_MPU6050_GRAVITY_MS2      9.80665f

/** @brief Amostra bruta (complemento de dois, 16 bits) lida dos registradores. */
typedef struct
{
    int16_t accel[3]; /**< ACCEL_XOUT, ACCEL_YOUT, ACCEL_ZOUT */
    int16_t temp;     /**< TEMP_OUT                           */
    int16_t gyro[3];  /**< GYRO_XOUT, GYRO_YOUT, GYRO_ZOUT    */
} bsp_mpu6050_raw_t;

/** @brief Amostra convertida para unidades de engenharia. */
typedef struct
{
    float accel_g[3];   /**< Aceleracao X, Y, Z em g                         */
    float accel_ms2[3]; /**< Aceleracao X, Y, Z em m/s^2                     */
    float gyro_dps[3];  /**< Velocidade angular X, Y, Z em graus/s (sem bias) */
    float temp_c;       /**< Temperatura do chip em graus Celsius            */
} bsp_mpu6050_data_t;

/**
 * @brief Detecta, reinicia e configura o MPU6050.
 * @return @c true se o sensor respondeu e todos os registradores foram escritos.
 */
bool bsp_mpu6050_init(void);

/**
 * @brief Le o registrador WHO_AM_I (0x75).
 * @return 0x68 para um MPU6050 original; 0x00 em erro de comunicacao.
 */
uint8_t bsp_mpu6050_who_am_i(void);

/**
 * @brief Le, em uma unica rajada de 14 bytes, aceleracao, temperatura e
 *        velocidade angular (registradores 0x3B a 0x48).
 *
 * A leitura em rajada garante que os 7 valores pertencem a mesma amostra.
 *
 * @param[out] raw Amostra bruta.
 * @return @c true em caso de sucesso.
 */
bool bsp_mpu6050_read_raw(bsp_mpu6050_raw_t *raw);

/**
 * @brief Converte uma amostra bruta para unidades de engenharia,
 *        descontando o bias do giroscopio obtido na calibracao.
 * @param[in]  raw Amostra bruta.
 * @param[out] out Amostra convertida.
 */
void bsp_mpu6050_convert(const bsp_mpu6050_raw_t *raw, bsp_mpu6050_data_t *out);

/**
 * @brief Le e converte uma amostra (@ref bsp_mpu6050_read_raw +
 *        @ref bsp_mpu6050_convert).
 * @param[out] out Amostra convertida.
 * @return @c true em caso de sucesso.
 */
bool bsp_mpu6050_read(bsp_mpu6050_data_t *out);

/**
 * @brief Estima o bias (offset) do giroscopio pela media de @p samples leituras.
 *
 * O sensor deve permanecer PARADO durante a chamada (bloqueante,
 * ~3 ms por amostra).
 *
 * @param samples Numero de amostras a promediar.
 * @return @c true se todas as leituras foram bem-sucedidas (bias atualizado).
 */
bool bsp_mpu6050_calibrate_gyro(uint16_t samples);

#endif /* BSP_MPU6050_H */
/** @} */
