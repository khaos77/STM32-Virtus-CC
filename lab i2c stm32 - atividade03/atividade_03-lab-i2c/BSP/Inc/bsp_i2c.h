/**
 * @file    bsp_i2c.h
 * @brief   Acesso ao barramento I2C1 (PB8 = SCL, PB9 = SDA, 400 kHz).
 *
 * @details Envolve as funcoes bloqueantes da HAL em uma interface orientada a
 *          registradores, que e o padrao de acesso tanto do MPU6050 quanto do
 *          SSD1306:
 *
 * @verbatim
 *  Escrita:  S | ADDR+W | A | REG | A | D0 | A | ... | Dn | A | P
 *  Leitura:  S | ADDR+W | A | REG | A | Sr | ADDR+R | A | D0 | A | ... | Dn | N | P
 * @endverbatim
 *
 *          Os enderecos sao sempre de 7 bits (o deslocamento exigido pela HAL
 *          e feito internamente).
 *
 * @defgroup BSP_I2C Barramento I2C
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_I2C_H
#define BSP_I2C_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/** @brief Timeout de cada transacao I2C (ms). */
#define BSP_I2C_TIMEOUT_MS   50U

/**
 * @brief Associa o handle do barramento usado pelos drivers de dispositivo.
 * @param hi2c Handle ja inicializado pelo CubeMX (@c MX_I2C1_Init).
 */
void bsp_i2c_init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Verifica se um dispositivo responde (ACK) no endereco informado.
 * @param addr7 Endereco I2C de 7 bits.
 * @return @c true se o dispositivo respondeu.
 */
bool bsp_i2c_is_ready(uint8_t addr7);

/**
 * @brief Escreve @p len bytes a partir do registrador @p reg.
 * @param addr7 Endereco I2C de 7 bits.
 * @param reg   Registrador inicial (ou byte de controle, no SSD1306).
 * @param data  Bytes a escrever.
 * @param len   Quantidade de bytes.
 * @return @c true em caso de sucesso.
 */
bool bsp_i2c_write_reg(uint8_t addr7, uint8_t reg, const uint8_t *data, uint16_t len);

/**
 * @brief Le @p len bytes a partir do registrador @p reg (leitura em rajada).
 * @param addr7 Endereco I2C de 7 bits.
 * @param reg   Registrador inicial.
 * @param data  Buffer de destino.
 * @param len   Quantidade de bytes.
 * @return @c true em caso de sucesso.
 */
bool bsp_i2c_read_reg(uint8_t addr7, uint8_t reg, uint8_t *data, uint16_t len);

/** @brief Numero acumulado de transacoes I2C que falharam desde o reset. */
uint32_t bsp_i2c_error_count(void);

#endif /* BSP_I2C_H */
/** @} */
