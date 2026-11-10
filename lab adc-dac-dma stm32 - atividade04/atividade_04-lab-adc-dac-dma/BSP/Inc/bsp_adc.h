/**
 * @file    bsp_adc.h
 * @brief   Leitura da tensao do divisor LDR/resistor com ADC1 + DMA circular.
 *
 * @details O ADC1 (canal IN3, PA3) converte continuamente e o DMA2_Stream0
 *          grava cada amostra em um buffer circular de
 *          @ref BSP_ADC_BUF_LEN posicoes, sem nenhuma intervencao da CPU:
 *
 * @verbatim
 *   PA3 --> ADC1 (12 bits, continuo) --DMA circular--> s_buf[64] --media--> bsp_adc_read_raw()
 * @endverbatim
 *
 *          - Clock do ADC: PCLK2/8 = 13,5 MHz.
 *          - Tempo de conversao: 480 + 12 = 492 ciclos -> ~27,4 kS/s.
 *          - Janela da media: 64 amostras ~ 2,3 ms (filtro de media movel).
 *
 *          O tempo de amostragem longo (480 ciclos) e necessario porque o
 *          divisor com o LDR tem impedancia de saida de varios kOhm.
 *
 * @defgroup BSP_ADC ADC (DMA)
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "main.h"
#include <stdbool.h>

/** @brief Numero de amostras do buffer circular preenchido pelo DMA. */
#define BSP_ADC_BUF_LEN   64U

/** @brief Fundo de escala do ADC de 12 bits. */
#define BSP_ADC_FULL_SCALE 4095U

/** @brief Tensao de referencia do ADC (VREF+ = VDDA na Nucleo), em mV. */
#define BSP_ADC_VREF_MV   3300U

/**
 * @brief Inicia as conversoes continuas com DMA circular.
 * @param hadc ADC ja configurado pelo CubeMX (ADC1).
 * @return @c true se a HAL aceitou o inicio das conversoes.
 * @note  As interrupcoes de meia/total transferencia do DMA sao desligadas:
 *        o buffer e apenas consultado pelo superloop, sem callbacks.
 */
bool bsp_adc_init(ADC_HandleTypeDef *hadc);

/**
 * @brief Media das @ref BSP_ADC_BUF_LEN amostras mais recentes.
 * @return Codigo do ADC, 0 a @ref BSP_ADC_FULL_SCALE.
 * @note  Nao bloqueia. Apos mudar a entrada, aguarde pelo menos ~3 ms para o
 *        buffer ser totalmente renovado.
 */
uint16_t bsp_adc_read_raw(void);

/**
 * @brief Converte um codigo do ADC em tensao.
 * @param raw Codigo de 12 bits.
 * @return Tensao em milivolts.
 */
uint32_t bsp_adc_raw_to_mv(uint16_t raw);

#endif /* BSP_ADC_H */
/** @} */
