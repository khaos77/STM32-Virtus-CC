/**
 * @file    bsp_timing.h
 * @brief   Base de tempo de alta resolucao usando o contador de ciclos DWT.
 *
 * @details O Cortex-M7 possui o registrador @c DWT->CYCCNT, que incrementa a
 *          cada ciclo de clock da CPU (216 MHz -> ~4,6 ns de resolucao).
 *          Ele e usado para:
 *          - atrasos em microssegundos durante a transmissao de bits;
 *          - medir o intervalo entre bordas de clock na recepcao;
 *          - medir o tempo total de transmissao de um pacote.
 *
 *          Vantagem: nenhum timer de hardware e consumido.
 *          O contador tem 32 bits e da a volta a cada ~19,9 s @ 216 MHz;
 *          diferencas calculadas com aritmetica sem sinal continuam corretas
 *          desde que o intervalo medido seja menor que isso.
 *
 * @defgroup BSP_TIMING Timing (DWT)
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_TIMING_H
#define BSP_TIMING_H

#include <stdint.h>

/** @brief Habilita e zera o contador de ciclos DWT. */
void bsp_timing_init(void);

/** @brief Le o valor atual do contador de ciclos (32 bits, com volta). */
uint32_t bsp_timing_cycles(void);

/** @brief Converte microssegundos em ciclos de CPU. */
uint32_t bsp_timing_us_to_cycles(uint32_t us);

/** @brief Converte ciclos de CPU em microssegundos (arredonda para baixo). */
uint32_t bsp_timing_cycles_to_us(uint32_t cycles);

/**
 * @brief Atraso ativo (busy-wait) em microssegundos.
 * @note  Se uma interrupcao ocorrer durante a espera, o atraso apenas fica
 *        maior - nunca menor - o que e seguro para o protocolo sincrono.
 */
void bsp_timing_delay_us(uint32_t us);

/** @brief Microssegundos decorridos desde @p start_cycles. */
uint32_t bsp_timing_elapsed_us(uint32_t start_cycles);

#endif /* BSP_TIMING_H */
/** @} */
