/**
 * @file    bsp_button.h
 * @brief   Botao de usuario (B1, azul) da NUCLEO-F767ZI - PC13 / EXTI13.
 *
 * @details A borda de subida (botao pressionado) gera uma interrupcao que
 *          apenas sinaliza um evento. O tratamento (medir o tempo segurado,
 *          disparar testes) e feito no laco principal, fora da ISR.
 *          Um filtro de debounce por tempo descarta repiques mecanicos.
 *
 * @defgroup BSP_BUTTON Botao
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdbool.h>
#include "main.h"

/** Pino EXTI do botao (usado pelo despacho em bsp.c). */
#define BSP_BUTTON_PIN          BTN_Pin
/** Janela de debounce em ms. */
#define BSP_BUTTON_DEBOUNCE_MS  50u

/** @brief Limpa eventos pendentes. */
void bsp_button_init(void);

/**
 * @brief Retorna @c true uma unica vez para cada pressionamento detectado.
 */
bool bsp_button_take_event(void);

/** @brief Nivel atual do botao (@c true = pressionado). */
bool bsp_button_is_pressed(void);

/** @brief Tratador chamado pelo despacho EXTI (nao chamar diretamente). */
void bsp_button_irq(void);

#endif /* BSP_BUTTON_H */
/** @} */
