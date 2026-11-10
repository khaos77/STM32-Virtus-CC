/**
 * @file    crc8.h
 * @brief   CRC-8 (polinomio 0x07, valor inicial 0x00 - "CRC-8/SMBUS").
 *
 * @details Detecta todos os erros de 1 e 2 bits e todos os erros em rajada
 *          de ate 8 bits dentro de um quadro. Calculado bit a bit (sem
 *          tabela) para economizar Flash; o custo e desprezivel para quadros
 *          de ate 259 bytes.
 *
 * @defgroup LINK_CRC CRC-8
 * @ingroup  LINK
 * @{
 */
#ifndef CRC8_H
#define CRC8_H

#include <stdint.h>
#include <stddef.h>

/** Polinomio gerador x^8 + x^2 + x + 1. */
#define CRC8_POLY  0x07u
/** Valor inicial do acumulador. */
#define CRC8_INIT  0x00u

/**
 * @brief Atualiza o CRC com um byte.
 * @param crc  CRC acumulado ate aqui.
 * @param byte Proximo byte.
 * @return Novo CRC.
 */
uint8_t crc8_update(uint8_t crc, uint8_t byte);

/**
 * @brief Calcula/continua o CRC sobre um buffer.
 * @param crc  CRC inicial (use ::CRC8_INIT ou o valor acumulado).
 * @param data Buffer.
 * @param len  Tamanho em bytes.
 * @return CRC resultante.
 */
uint8_t crc8_compute(uint8_t crc, const uint8_t *data, size_t len);

#endif /* CRC8_H */
/** @} */
