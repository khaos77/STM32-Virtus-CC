/**
 * @file    attitude.h
 * @brief   Estimador de atitude (roll, pitch, yaw) por filtro complementar.
 *
 * @details Modulo puramente matematico: nao depende de HAL nem de BSP.
 *
 * Convencao de eixos (a do proprio MPU6050, regra da mao direita, Z para cima
 * com o sensor na horizontal): roll @f$\phi@f$ em torno de X, pitch
 * @f$\theta@f$ em torno de Y e yaw @f$\psi@f$ em torno de Z.
 *
 * **1. Angulos pelo acelerometro** (referencia de longo prazo, ruidosa):
 * @f[ \phi_{acc} = \operatorname{atan2}(a_y,\ a_z) \qquad
 *     \theta_{acc} = \operatorname{atan2}\!\left(-a_x,\ \sqrt{a_y^2 + a_z^2}\right) @f]
 *
 * **2. Taxas de Euler pelo giroscopio** (resposta rapida, com deriva):
 * @f[ \dot\phi   = p + (q\sin\phi + r\cos\phi)\tan\theta @f]
 * @f[ \dot\theta = q\cos\phi - r\sin\phi @f]
 * @f[ \dot\psi   = \frac{q\sin\phi + r\cos\phi}{\cos\theta} @f]
 * onde @f$(p, q, r)@f$ sao as velocidades angulares medidas em X, Y e Z.
 *
 * **3. Fusao (filtro complementar)**: passa-altas no giroscopio somado a
 * passa-baixas no acelerometro:
 * @f[ \phi_k   = \alpha\,(\phi_{k-1}   + \dot\phi\,\Delta t)   + (1-\alpha)\,\phi_{acc} @f]
 * @f[ \theta_k = \alpha\,(\theta_{k-1} + \dot\theta\,\Delta t) + (1-\alpha)\,\theta_{acc} @f]
 * @f[ \psi_k   = \psi_{k-1} + \dot\psi\,\Delta t @f]
 *
 * @verbatim
 *   giroscopio --> [taxas de Euler] --> [ integra: +w*dt ] --(alpha)----+
 *                                              ^                        v
 *                                              |                      ( + ) --> angulo
 *                                              +---- angulo anterior    ^
 *   acelerometro --> [ atan2 ] ----------------------------(1-alpha)----+
 * @endverbatim
 *
 * @note O acelerometro nao observa rotacao em torno da vertical, portanto o
 *       **yaw e apenas a integral do giroscopio**: ele e relativo a posicao
 *       inicial e deriva lentamente (seria necessario um magnetometro para
 *       corrigi-lo).
 * @note Perto de @f$\theta = \pm 90^\circ@f$ (gimbal lock) roll e yaw deixam
 *       de ser distinguiveis; a correcao de roll pelo acelerometro e
 *       suspensa e @f$\cos\theta@f$ e limitado para evitar divisao por zero.
 *
 * @defgroup ATTITUDE Atitude - Filtro complementar
 * @ingroup  APP
 * @{
 */
#ifndef ATTITUDE_H
#define ATTITUDE_H

#include <stdbool.h>

/** @brief Estado do estimador de atitude. Angulos em graus. */
typedef struct
{
    float roll;        /**< Rolagem  @f$\phi@f$,   -180..+180 graus      */
    float pitch;       /**< Arfagem  @f$\theta@f$,  -90..+90 graus       */
    float yaw;         /**< Guinada  @f$\psi@f$,   -180..+180 graus      */
    float alpha;       /**< Peso do giroscopio no filtro (0..1)          */
    bool  initialized; /**< @c false ate a primeira amostra ser recebida */
} attitude_t;

/**
 * @brief Zera o estimador.
 * @param att   Estado do estimador.
 * @param alpha Ganho do filtro complementar (tipicamente 0,95 a 0,99).
 */
void attitude_init(attitude_t *att, float alpha);

/**
 * @brief Executa um passo do filtro complementar.
 *
 * Na primeira chamada roll e pitch sao inicializados diretamente pelo
 * acelerometro, evitando o transitorio de convergencia.
 *
 * @param att      Estado do estimador (atualizado).
 * @param accel_g  Aceleracao X, Y, Z em g.
 * @param gyro_dps Velocidade angular X, Y, Z em graus/s.
 * @param dt       Tempo decorrido desde o passo anterior, em segundos.
 */
void attitude_update(attitude_t *att, const float accel_g[3],
                     const float gyro_dps[3], float dt);

/** @brief Define a orientacao atual como referencia de yaw (yaw = 0). */
void attitude_reset_yaw(attitude_t *att);

#endif /* ATTITUDE_H */
/** @} */
