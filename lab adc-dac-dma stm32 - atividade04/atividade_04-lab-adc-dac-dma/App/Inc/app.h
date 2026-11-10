/**
 * @file    app.h
 * @brief   Aplicacao do Lab ADC-DAC-DMA: digipot com LED + LDR.
 *
 * @details Superloop bare-metal cooperativo:
 *
 * @verbatim
 *   app_loop()
 *     |-- heartbeat    LED verde a cada 500 ms
 *     |-- cli_task()   comandos de texto e carga da LUT (UART RX por DMA)
 *     |-- ohm_ctrl_task()  aplicacao exemplo: malha fechada em ohms (ADC por DMA)
 *     `-- LEDs         azul = LUT calibrada (ou malha travada); vermelho = erro
 * @endverbatim
 *
 * @defgroup APP App - Aplicacao
 * @{
 */
#ifndef APP_H
#define APP_H

#include <stdbool.h>

/**
 * @brief Inicializa o digipot e a linha de comando; imprime o cabecalho.
 * @param bsp_ok Resultado de bsp_init(); @c false acende o LED vermelho.
 */
void app_init(bool bsp_ok);

/** @brief Uma iteracao do superloop. */
void app_loop(void);

#endif /* APP_H */
/** @} */
