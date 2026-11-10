/**
 * @file    bsp_timing.c
 * @brief   Implementacao da base de tempo DWT.
 * @ingroup BSP_TIMING
 */
#include "bsp_timing.h"
#include "main.h"

/** Ciclos de CPU por microssegundo (216 para HCLK = 216 MHz). */
static uint32_t s_cycles_per_us = 1u;

void bsp_timing_init(void)
{
    s_cycles_per_us = SystemCoreClock / 1000000u;
    if (s_cycles_per_us == 0u)
    {
        s_cycles_per_us = 1u;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* liga o bloco de trace */
    DWT->LAR = 0xC5ACCE55u;                         /* Cortex-M7: destrava escrita no DWT */
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            /* inicia a contagem */
}

uint32_t bsp_timing_cycles(void)
{
    return DWT->CYCCNT;
}

uint32_t bsp_timing_us_to_cycles(uint32_t us)
{
    return us * s_cycles_per_us;
}

uint32_t bsp_timing_cycles_to_us(uint32_t cycles)
{
    return cycles / s_cycles_per_us;
}

void bsp_timing_delay_us(uint32_t us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t wait  = us * s_cycles_per_us;

    while ((DWT->CYCCNT - start) < wait)
    {
        /* espera ativa */
    }
}

uint32_t bsp_timing_elapsed_us(uint32_t start_cycles)
{
    return (DWT->CYCCNT - start_cycles) / s_cycles_per_us;
}
