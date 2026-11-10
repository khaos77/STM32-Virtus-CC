/**
 * @file    bsp.h
 * @brief   Ponto de entrada unico da BSP (Board Support Package) da NUCLEO-F767ZI.
 *
 * @details A BSP e a unica camada que conhece pinos, perifericos e os
 *          dispositivos I2C ligados a placa (MPU6050 e SSD1306). A camada App
 *          so fala com o hardware atraves das funcoes declaradas nos headers
 *          incluidos aqui.
 *
 * @verbatim
 *   +---------------------------------------------+
 *   |  App  (app.c, attitude.c, ui.c)             |  filtro complementar, telas
 *   +---------------------------------------------+
 *   |  BSP - dispositivos                         |
 *   |    bsp_mpu6050.c        bsp_ssd1306.c       |  drivers dos CIs I2C
 *   +---------------------------------------------+
 *   |  BSP - placa                                |
 *   |    bsp_i2c  bsp_timing  bsp_led             |
 *   |    bsp_button  bsp_console                  |  barramento, DWT, GPIO, UART
 *   +---------------------------------------------+
 *   |  STM32 HAL + codigo gerado pelo CubeMX      |
 *   +---------------------------------------------+
 * @endverbatim
 *
 * Ligacoes (conector Arduino CN7 da NUCLEO-F767ZI):
 *
 * | Sinal      | Pino MCU | Conector | MPU6050 | SSD1306 |
 * |------------|----------|----------|---------|---------|
 * | I2C1_SCL   | PB8      | D15      | SCL     | SCL     |
 * | I2C1_SDA   | PB9      | D14      | SDA     | SDA     |
 * | 3V3        | -        | 3V3      | VCC     | VCC     |
 * | GND        | -        | GND      | GND/AD0 | GND     |
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
#include "bsp_timing.h"
#include "bsp_console.h"
#include "bsp_led.h"
#include "bsp_button.h"
#include "bsp_i2c.h"
#include "bsp_mpu6050.h"
#include "bsp_ssd1306.h"

/**
 * @brief Inicializa os modulos de placa da BSP.
 *
 * Deve ser chamada uma vez, depois de @c MX_GPIO_Init(), @c MX_I2C1_Init() e
 * @c MX_USART3_UART_Init() (geradas pelo CubeMX).
 *
 * @note Os dispositivos I2C (MPU6050 e SSD1306) NAO sao inicializados aqui:
 *       eles podem estar ausentes ou mal conectados, entao a aplicacao chama
 *       @ref bsp_mpu6050_init e @ref bsp_ssd1306_init e trata a falha.
 *
 * @param hi2c          Barramento I2C compartilhado pelos dois dispositivos.
 * @param huart_console UART usada para log (@c printf). Pode ser @c NULL.
 */
void bsp_init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart_console);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
/** @} */
