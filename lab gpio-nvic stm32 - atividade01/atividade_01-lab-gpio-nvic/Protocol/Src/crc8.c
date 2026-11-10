/**
 * @file    crc8.c
 * @brief   Implementacao do CRC-8 bit a bit.
 * @ingroup LINK_CRC
 */
#include "crc8.h"

uint8_t crc8_update(uint8_t crc, uint8_t byte)
{
    crc ^= byte;
    for (uint8_t i = 0; i < 8u; i++)
    {
        crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ CRC8_POLY) : (uint8_t)(crc << 1);
    }
    return crc;
}

uint8_t crc8_compute(uint8_t crc, const uint8_t *data, size_t len)
{
    while (len--)
    {
        crc = crc8_update(crc, *data++);
    }
    return crc;
}
