/**
 * @file    bsp_link_io.h
 * @brief   Linhas fisicas do link GPIO entre as duas placas.
 *
 * @details Cada placa possui um par de saida (TX) e um par de entrada (RX).
 *          As placas sao ligadas em cruz:
 *
 * | Sinal   | Pino NUCLEO | Header | Modo                      | Liga na outra placa em |
 * |---------|-------------|--------|---------------------------|------------------------|
 * | TX_CLK  | PF15        | D2     | Saida push-pull           | RX_CLK (D4)            |
 * | TX_DATA | PE13        | D3     | Saida push-pull           | RX_DATA (D5)           |
 * | RX_CLK  | PF14        | D4     | EXTI14 borda de subida, pull-down | TX_CLK (D2)    |
 * | RX_DATA | PE11        | D5     | Entrada, pull-down        | TX_DATA (D3)           |
 * | GND     | GND         | -      | -                         | GND (obrigatorio)      |
 *
 *          Os pull-downs garantem nivel baixo (repouso) se a outra placa
 *          estiver desligada ou em reset, evitando bordas falsas no RX_CLK.
 *
 * @defgroup BSP_LINK_IO Link IO
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_LINK_IO_H
#define BSP_LINK_IO_H

#include <stdint.h>
#include "main.h"

/** Pino EXTI do clock de recepcao (usado pelo despacho em bsp.c). */
#define BSP_LINK_RX_CLK_PIN  RX_CLK_Pin

/** @brief Tipo da funcao chamada a cada borda de subida em RX_CLK. */
typedef void (*bsp_link_rx_clk_cb_t)(void);

/**
 * @brief (Re)aplica a configuracao eletrica das 4 linhas do link.
 *
 * @details O CubeMX ja configura os pinos, mas esta funcao garante a
 *          configuracao exigida pelo protocolo (pull-down nas entradas,
 *          velocidade maxima nas saidas, linhas em repouso = 0) mesmo que o
 *          .ioc seja alterado por engano. Assim a BSP e autossuficiente.
 */
void bsp_link_io_init(void);

/** @brief Registra a callback de borda de subida do RX_CLK (camada Protocol). */
void bsp_link_io_set_rx_clk_callback(bsp_link_rx_clk_cb_t cb);

/** @brief Escreve o nivel da linha TX_CLK. */
void bsp_link_io_tx_clk_write(uint8_t level);

/** @brief Escreve o nivel da linha TX_DATA. */
void bsp_link_io_tx_data_write(uint8_t level);

/** @brief Le o nivel da linha RX_CLK (0 ou 1). */
uint8_t bsp_link_io_rx_clk_read(void);

/** @brief Le o nivel da linha RX_DATA (0 ou 1). */
uint8_t bsp_link_io_rx_data_read(void);

/** @brief Tratador chamado pelo despacho EXTI (nao chamar diretamente). */
void bsp_link_io_rx_clk_irq(void);

#endif /* BSP_LINK_IO_H */
/** @} */
