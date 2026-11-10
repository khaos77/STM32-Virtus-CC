/**
 * @file    bsp_button.h
 * @brief   Botao de usuario B1 (PC13, ativo em nivel alto) com debounce.
 *
 * @details O botao e lido por varredura (polling) no superloop; o debounce e a
 *          classificacao clique curto / pressionamento longo sao feitos por
 *          tempo, sem bloquear.
 *
 * @defgroup BSP_BUTTON Botao
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

/** @brief Tempo de estabilidade exigido para aceitar uma mudanca (ms). */
#define BSP_BUTTON_DEBOUNCE_MS   30U
/** @brief Tempo minimo pressionado para gerar @ref BSP_BUTTON_EVT_LONG (ms). */
#define BSP_BUTTON_LONG_MS       1000U

/** @brief Eventos gerados pelo botao. */
typedef enum
{
    BSP_BUTTON_EVT_NONE = 0, /**< Nada aconteceu                              */
    BSP_BUTTON_EVT_SHORT,    /**< Clique curto (gerado ao soltar)             */
    BSP_BUTTON_EVT_LONG      /**< Pressionado por >= @ref BSP_BUTTON_LONG_MS  */
} bsp_button_evt_t;

/** @brief Zera a maquina de estados do botao. */
void bsp_button_init(void);

/**
 * @brief Varre o botao; deve ser chamada continuamente no superloop.
 * @return Evento detectado nesta chamada (cada evento e entregue uma unica vez).
 */
bsp_button_evt_t bsp_button_poll(void);

#endif /* BSP_BUTTON_H */
/** @} */
