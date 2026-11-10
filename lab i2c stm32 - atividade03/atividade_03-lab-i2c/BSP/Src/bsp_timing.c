/**
 * @file    bsp_timing.c
 * @brief   Implementacao da base de tempo (DWT + SysTick).
 * @ingroup BSP_TIMING
 */
#include "bsp_timing.h"
#include "main.h"

void bsp_timing_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* liga o bloco de trace */
    DWT->LAR = 0xC5ACCE55U;                         /* destrava o DWT (Cortex-M7) */
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t bsp_timing_cycles(void)
{
    return DWT->CYCCNT;
}

uint32_t bsp_timing_elapsed_us(uint32_t start_cycles)
{
    uint32_t cycles = DWT->CYCCNT - start_cycles; /* sem sinal: correto na volta */
    return cycles / (SystemCoreClock / 1000000U);
}

uint32_t bsp_timing_millis(void)
{
    return HAL_GetTick();
}

void bsp_timing_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
