/**
 * @file    bsp_timing.h
 * @brief   Base de tempo de alta resolucao usando o contador de ciclos DWT.
 *
 * @details O Cortex-M7 possui o registrador @c DWT->CYCCNT, que incrementa a
 *          cada ciclo de clock da CPU (216 MHz -> ~4,6 ns de resolucao).
 *          Ele e usado para medir o intervalo real @c dt entre duas amostras
 *          do MPU6050, que entra diretamente na integracao do giroscopio do
 *          filtro complementar.
 *
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

/** @brief Microssegundos decorridos desde @p start_cycles (valido ate ~19 s). */
uint32_t bsp_timing_elapsed_us(uint32_t start_cycles);

/** @brief Milissegundos desde o reset (SysTick da HAL). */
uint32_t bsp_timing_millis(void);

/** @brief Atraso bloqueante em milissegundos. */
void bsp_timing_delay_ms(uint32_t ms);

#endif /* BSP_TIMING_H */
/** @} */
