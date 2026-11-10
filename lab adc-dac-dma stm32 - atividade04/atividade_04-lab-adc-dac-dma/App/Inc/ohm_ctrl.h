/**
 * @file    ohm_ctrl.h
 * @brief   Aplicacao exemplo do digipot: resistencia programavel em ohms
 *          ("decada resistiva") com malha fechada.
 *
 * @details Em vez de escolher uma palavra digital, o usuario pede um valor
 *          em ohms (comando @c ohm). Um controlador integral ajusta o DAC ate
 *          que a resistencia medida pelo ADC seja igual ao alvo, compensando
 *          deriva termica e envelhecimento do LED/LDR. A LUT calibrada e
 *          usada como @e feedforward (ponto de partida).
 *
 * @verbatim
 *   R_alvo --->(+)--> e = (R_med - R_alvo)/R_alvo --> [ Ki ] --> (+)--> DAC --> LED ~> LDR --+
 *               ^-                                                ^                          |
 *               |                                    LUT[res(R_alvo)] (feedforward)          |
 *               +------------- R_med = Rs*(4095-N)/N <------------- ADC + DMA <--------------+
 * @endverbatim
 *
 *          Lei de controle, executada a cada @c APP_OHM_PERIOD_MS:
 *
 *          @f[ e_k = \frac{R_{med,k} - R_{alvo}}{R_{alvo}}, \qquad
 *              DAC_{k+1} = DAC_k + K_i\,e_k @f]
 *
 *          (mais luz -> menos resistencia, por isso erro positivo aumenta o DAC).
 *          O erro relativo torna o ganho de malha parecido em toda a faixa,
 *          ja que a resistencia do LDR varia em decadas.
 *
 * @defgroup OHM_CTRL Aplicacao exemplo - resistencia em ohms
 * @ingroup  APP
 * @{
 */
#ifndef OHM_CTRL_H
#define OHM_CTRL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Liga a malha com um novo alvo.
 * @param r_ohm Resistencia desejada em ohms (> 0).
 */
void ohm_ctrl_start(uint32_t r_ohm);

/** @brief Desliga a malha; o DAC permanece no ultimo valor. */
void ohm_ctrl_stop(void);

/** @brief @c true enquanto a malha esta ligada. */
bool ohm_ctrl_active(void);

/** @brief @c true se a malha esta ligada e o erro esta abaixo da tolerancia. */
bool ohm_ctrl_locked(void);

/** @brief Passo do controlador; chamar a cada iteracao do superloop. */
void ohm_ctrl_task(void);

#endif /* OHM_CTRL_H */
/** @} */
