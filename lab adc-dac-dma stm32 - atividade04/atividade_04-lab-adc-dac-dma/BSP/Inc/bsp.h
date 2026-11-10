/**
 * @file    bsp.h
 * @brief   Ponto de entrada unico da BSP (Board Support Package) da NUCLEO-F767ZI.
 *
 * @details A BSP e a unica camada que conhece pinos, perifericos e a HAL.
 *          A camada App (digipot, linha de comando, aplicacao exemplo) so fala
 *          com o hardware atraves das funcoes @c bsp_* declaradas nos headers
 *          incluidos aqui.
 *
 * @verbatim
 *   +-----------------------------------------------------+
 *   |  App  (app.c, digipot.c, cli.c, ohm_ctrl.c)         |  LUT, calibracao, comandos
 *   +-----------------------------------------------------+
 *   |  BSP                                                |
 *   |    bsp_adc (DMA)   bsp_dac   bsp_console (DMA)      |
 *   |    bsp_led         bsp (tempo)                      |
 *   +-----------------------------------------------------+
 *   |  STM32 HAL + codigo gerado pelo CubeMX (Core/)      |
 *   +-----------------------------------------------------+
 * @endverbatim
 *
 * Mapa de hardware (NUCLEO-F767ZI):
 *
 * | Funcao              | Pino | Periferico                    | Conector          |
 * |---------------------|------|-------------------------------|-------------------|
 * | Tensao do divisor   | PA3  | ADC1_IN3 + DMA2_Stream0       | CN9 pino 1 (A0)   |
 * | Acionamento do LED  | PA4  | DAC_OUT1                      | CN7 pino 17 (D24) |
 * | Console TX          | PD8  | USART3 (ST-LINK VCP)          | USB do ST-LINK    |
 * | Console RX          | PD9  | USART3 + DMA1_Stream1         | USB do ST-LINK    |
 * | LED verde (LD1)     | PB0  | GPIO                          | -                 |
 * | LED azul (LD2)      | PB7  | GPIO                          | -                 |
 * | LED vermelho (LD3)  | PB14 | GPIO                          | -                 |
 *
 * @defgroup BSP BSP - Board Support Package
 * @brief    Abstracao do hardware da NUCLEO-F767ZI usando a HAL.
 * @{
 */
#ifndef BSP_H
#define BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "bsp_led.h"
#include "bsp_console.h"
#include "bsp_adc.h"
#include "bsp_dac.h"

/**
 * @brief Inicializa todos os modulos da BSP.
 *
 * Deve ser chamada uma vez, depois dos @c MX_*_Init() gerados pelo CubeMX
 * (GPIO, DMA, ADC1, DAC e USART3).
 *
 * @param hadc  ADC do divisor de tensao (ADC1, modo continuo + DMA circular).
 * @param hdac  DAC que aciona o LED do digipot (canal 1).
 * @param huart UART do console (USART3, recepcao por DMA circular).
 * @return @c true se ADC, DAC e console iniciaram sem erro.
 */
bool bsp_init(ADC_HandleTypeDef *hadc, DAC_HandleTypeDef *hdac,
              UART_HandleTypeDef *huart);

/**
 * @brief Tempo desde o reset, em milissegundos (base SysTick).
 * @return Contador de 32 bits (estoura a cada ~49 dias).
 */
uint32_t bsp_millis(void);

/**
 * @brief Espera bloqueante.
 * @param ms Duracao em milissegundos.
 */
void bsp_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
/** @} */
