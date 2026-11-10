/**
 * @file    bsp_dac.h
 * @brief   Saida analogica que aciona o LED do digipot (DAC_OUT1, PA4).
 *
 * @details DAC de 12 bits com buffer de saida habilitado e sem trigger: o
 *          valor escrito aparece imediatamente no pino.
 *
 *          @f[ V_{DAC} = \frac{code}{4095}\cdot V_{REF+} \qquad (V_{REF+} = 3{,}3\,V) @f]
 *
 * @warning O buffer do DAC fornece pouca corrente (carga minima de 5 kOhm
 *          segundo o datasheet). Para mais de ~1 mA no LED use um seguidor
 *          de emissor (ex.: BC547) ou um amp-op como driver; a calibracao da
 *          LUT absorve a queda de tensao do driver.
 *
 * @defgroup BSP_DAC DAC
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_DAC_H
#define BSP_DAC_H

#include "main.h"
#include <stdbool.h>

/** @brief Maior codigo aceito pelo DAC de 12 bits. */
#define BSP_DAC_MAX_CODE 4095U

/**
 * @brief Habilita o canal 1 do DAC com saida em 0 V (LED apagado).
 * @param hdac DAC ja configurado pelo CubeMX.
 * @return @c true se a HAL aceitou a habilitacao.
 */
bool bsp_dac_init(DAC_HandleTypeDef *hdac);

/**
 * @brief Escreve um codigo no DAC.
 * @param code Codigo de 12 bits; valores acima de @ref BSP_DAC_MAX_CODE
 *             sao saturados.
 */
void bsp_dac_write(uint16_t code);

/** @brief Ultimo codigo escrito no DAC. */
uint16_t bsp_dac_read(void);

/**
 * @brief Converte um codigo do DAC na tensao nominal de saida.
 * @param code Codigo de 12 bits.
 * @return Tensao em milivolts.
 */
uint32_t bsp_dac_code_to_mv(uint16_t code);

#endif /* BSP_DAC_H */
/** @} */
