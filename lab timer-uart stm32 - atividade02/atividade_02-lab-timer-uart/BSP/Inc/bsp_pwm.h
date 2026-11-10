/**
 * @file    bsp_pwm.h
 * @brief   Saida PWM (TIM3_CH1 em PA6) usada como portadora do gerador senoidal.
 *
 * @details Configuracao feita no CubeMX (.ioc):
 *          - Clock do timer: 108 MHz (APB1 = 54 MHz, timers x2)
 *          - PSC = 0, ARR = 1079  ->  f_pwm = 108 MHz / 1080 = 100 kHz
 *          - Resolucao do duty: 1080 niveis (~10 bits)
 *          - Preload de ARR e CCR1 habilitado: o novo duty so entra em vigor
 *            no evento de update, sem glitch no meio do periodo
 *          - Interrupcao de update (TIM3_IRQn) habilitada: a cada periodo de
 *            PWM o BSP chama o callback registrado, que calcula o proximo duty
 *
 * @defgroup BSP_PWM PWM
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "main.h"

/**
 * @brief Callback executado a cada periodo de PWM (contexto de interrupcao).
 * @warning Deve ser curto: roda a 100 kHz.
 */
typedef void (*bsp_pwm_update_cb_t)(void);

/**
 * @brief Associa o timer de PWM ao BSP (nao inicia a saida).
 * @param htim Timer ja configurado pelo CubeMX.
 */
void bsp_pwm_init(TIM_HandleTypeDef *htim);

/**
 * @brief Registra a funcao chamada a cada evento de update do timer.
 * @param cb Callback, ou @c NULL para remover.
 */
void bsp_pwm_set_update_callback(bsp_pwm_update_cb_t cb);

/** @brief Inicia a saida PWM e a interrupcao de update. */
void bsp_pwm_start(void);

/** @brief Para a saida PWM (pino em nivel baixo) e a interrupcao de update. */
void bsp_pwm_stop(void);

/**
 * @brief Numero de contagens de um periodo de PWM (ARR + 1).
 * @return Valor de compare que corresponde a 100 % de duty.
 */
uint32_t bsp_pwm_get_period_counts(void);

/**
 * @brief Frequencia da portadora PWM, calculada a partir do clock real.
 * @return Frequencia em Hz (tambem e a taxa de chamada do callback).
 */
uint32_t bsp_pwm_get_carrier_hz(void);

/**
 * @brief Define o duty cycle em contagens do timer.
 * @param compare Valor de 0 (0 %) a bsp_pwm_get_period_counts() (100 %).
 * @note  Efetivado no proximo update (preload), pode ser chamado na ISR.
 */
void bsp_pwm_set_compare(uint32_t compare);

#endif /* BSP_PWM_H */
/** @} */
