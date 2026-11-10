/**
 * @file    bsp.c
 * @brief   Inicializacao da BSP e despacho centralizado das interrupcoes EXTI.
 * @ingroup BSP
 */
#include "bsp.h"

void bsp_init(UART_HandleTypeDef *huart_console)
{
    bsp_timing_init();
    bsp_console_init(huart_console);
    bsp_led_init();
    bsp_button_init();
    bsp_link_io_init();
}

/**
 * @brief Callback da HAL chamada por @c HAL_GPIO_EXTI_IRQHandler().
 *
 * @details No STM32F767 as linhas EXTI10..15 compartilham o mesmo vetor
 *          (@c EXTI15_10_IRQn). A HAL ja limpa o flag pendente e informa qual
 *          pino gerou o evento; aqui apenas repassamos para o modulo dono do
 *          pino. Manter um unico ponto de despacho evita que dois modulos
 *          tentem definir esta funcao (ela e @c weak na HAL).
 *
 * @param GPIO_Pin Mascara do pino que gerou a interrupcao.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BSP_LINK_RX_CLK_PIN)
    {
        bsp_link_io_rx_clk_irq();   /* caminho critico: primeiro */
    }
    else if (GPIO_Pin == BSP_BUTTON_PIN)
    {
        bsp_button_irq();
    }
}
