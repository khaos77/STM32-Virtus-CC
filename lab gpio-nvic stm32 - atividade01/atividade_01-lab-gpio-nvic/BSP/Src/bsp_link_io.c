/**
 * @file    bsp_link_io.c
 * @brief   Implementacao das linhas fisicas do link usando HAL_GPIO.
 * @ingroup BSP_LINK_IO
 */
#include "bsp_link_io.h"

static bsp_link_rx_clk_cb_t s_rx_clk_cb = NULL; /**< Callback da camada Protocol */

void bsp_link_io_init(void)
{
    GPIO_InitTypeDef cfg = {0};

    /* Saidas em repouso (nivel baixo) antes de qualquer coisa. */
    HAL_GPIO_WritePin(TX_CLK_GPIO_Port,  TX_CLK_Pin,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TX_DATA_GPIO_Port, TX_DATA_Pin, GPIO_PIN_RESET);

    /* TX_CLK e TX_DATA: push-pull, velocidade maxima (bordas limpas). */
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    cfg.Pin   = TX_CLK_Pin;
    HAL_GPIO_Init(TX_CLK_GPIO_Port, &cfg);
    cfg.Pin   = TX_DATA_Pin;
    HAL_GPIO_Init(TX_DATA_GPIO_Port, &cfg);

    /* RX_DATA: entrada digital com pull-down. */
    cfg.Mode  = GPIO_MODE_INPUT;
    cfg.Pull  = GPIO_PULLDOWN;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    cfg.Pin   = RX_DATA_Pin;
    HAL_GPIO_Init(RX_DATA_GPIO_Port, &cfg);

    /* RX_CLK: interrupcao externa na borda de subida, com pull-down. */
    cfg.Mode  = GPIO_MODE_IT_RISING;
    cfg.Pull  = GPIO_PULLDOWN;
    cfg.Pin   = RX_CLK_Pin;
    HAL_GPIO_Init(RX_CLK_GPIO_Port, &cfg);

    /* Maior prioridade do sistema: a amostragem do bit nao pode atrasar. */
    HAL_NVIC_SetPriority(RX_CLK_EXTI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RX_CLK_EXTI_IRQn);
}

void bsp_link_io_set_rx_clk_callback(bsp_link_rx_clk_cb_t cb)
{
    s_rx_clk_cb = cb;
}

void bsp_link_io_tx_clk_write(uint8_t level)
{
    HAL_GPIO_WritePin(TX_CLK_GPIO_Port, TX_CLK_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bsp_link_io_tx_data_write(uint8_t level)
{
    HAL_GPIO_WritePin(TX_DATA_GPIO_Port, TX_DATA_Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t bsp_link_io_rx_clk_read(void)
{
    return (HAL_GPIO_ReadPin(RX_CLK_GPIO_Port, RX_CLK_Pin) == GPIO_PIN_SET) ? 1u : 0u;
}

uint8_t bsp_link_io_rx_data_read(void)
{
    return (HAL_GPIO_ReadPin(RX_DATA_GPIO_Port, RX_DATA_Pin) == GPIO_PIN_SET) ? 1u : 0u;
}

void bsp_link_io_rx_clk_irq(void)
{
    if (s_rx_clk_cb != NULL)
    {
        s_rx_clk_cb();
    }
}
