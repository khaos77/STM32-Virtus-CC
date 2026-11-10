/**
 * @file    app.h
 * @brief   Aplicacao do Lab I2C: estimacao de atitude (roll, pitch, yaw).
 *
 * @details Superloop bare-metal cooperativo:
 *
 * @verbatim
 *            +-----------+  falha   +--------------------+
 *   reset -> |  INIT     | -------> |  ERRO (LED verm.)  |
 *            | SSD1306 + | <------- |  nova tentativa a  |
 *            | MPU6050   |  500 ms  |  cada 500 ms       |
 *            +-----------+          +--------------------+
 *                 | ok                        ^
 *                 v                           | 20 leituras falhas
 *            +-----------+   ok     +--------------------+
 *            | CALIBRA   | -------> |  RUN               |
 *            | giroscopio|          |  200 Hz: le MPU,   |
 *            | (parado)  | <------- |  filtra, desenha   |
 *            +-----------+  botao   +--------------------+
 *                           longo
 * @endverbatim
 *
 * | Botao B1 (azul)   | Acao                                                  |
 * |-------------------|-------------------------------------------------------|
 * | clique curto      | alterna entre a tela de dados e a tela de animacao    |
 * | segurar >= 1 s    | recalibra o giroscopio e zera o yaw (manter parado)   |
 *
 * LEDs: verde pisca = rodando; azul = tela de animacao; vermelho = erro I2C.
 *
 * @defgroup APP App - Aplicacao
 * @{
 */
#ifndef APP_H
#define APP_H

/** @brief Inicializa display, sensor e filtro; imprime o cabecalho no console. */
void app_init(void);

/** @brief Uma iteracao do superloop (no maximo ~4 ms por chamada). */
void app_loop(void);

#endif /* APP_H */
/** @} */
