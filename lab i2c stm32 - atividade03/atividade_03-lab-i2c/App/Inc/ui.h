/**
 * @file    ui.h
 * @brief   Telas do display OLED (dados numericos e animacao).
 *
 * @details As funcoes apenas desenham no framebuffer da BSP
 *          (@ref BSP_SSD1306); quem envia o quadro ao display e a aplicacao.
 *
 * Tela de dados (21 x 8 caracteres):
 * @verbatim
 *    ACEL[m/s2] GIRO[d/s]
 *   X  +0.12     +0.3
 *   Y  -0.05     -1.2
 *   Z  +9.79     +0.0
 *   ---------------------
 *   ROLL    +0.5°
 *   PITCH   -1.2°
 *   YAW    +90.0°
 * @endverbatim
 *
 * Tela de animacao: horizonte artificial (roll + pitch) a esquerda e
 * bussola (yaw) a direita.
 *
 * @defgroup UI UI - Telas
 * @ingroup  APP
 * @{
 */
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "attitude.h"
#include "bsp_mpu6050.h"

/** @brief Telas disponiveis. */
typedef enum
{
    UI_SCREEN_DATA = 0, /**< Aceleracao, velocidade angular e angulos (texto) */
    UI_SCREEN_ANIM,     /**< Horizonte artificial + bussola (animacao)        */
    UI_SCREEN_COUNT
} ui_screen_t;

/**
 * @brief Formata um numero real com sinal, alinhado a direita, sem depender
 *        do suporte a @c float do @c printf.
 * @param[out] buf      Destino, com pelo menos @p width + 1 bytes.
 * @param      value    Valor a formatar.
 * @param      decimals Casas decimais (0 a 3).
 * @param      width    Largura total do campo, em caracteres (maximo 15).
 */
void ui_format_fixed(char *buf, float value, uint8_t decimals, uint8_t width);

/**
 * @brief Desenha uma tela de mensagem com ate tres linhas centralizadas.
 * @param l1 Primeira linha (ou @c NULL).
 * @param l2 Segunda linha (ou @c NULL).
 * @param l3 Terceira linha (ou @c NULL).
 */
void ui_draw_message(const char *l1, const char *l2, const char *l3);

/**
 * @brief Desenha a tela de dados: aceleracao linear (3 eixos), velocidade
 *        angular (3 eixos) e angulos de roll, pitch e yaw.
 */
void ui_draw_data(const bsp_mpu6050_data_t *imu, const attitude_t *att);

/**
 * @brief Desenha a tela de animacao (requisito avancado): horizonte
 *        artificial girado pelo roll e deslocado pelo pitch, e bussola
 *        cujo ponteiro indica o yaw.
 */
void ui_draw_anim(const attitude_t *att);

#endif /* UI_H */
/** @} */
