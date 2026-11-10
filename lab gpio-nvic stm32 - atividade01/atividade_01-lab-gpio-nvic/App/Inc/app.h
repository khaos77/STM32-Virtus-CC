/**
 * @file    app.h
 * @brief   Aplicacao de teste do GPIO-Link.
 *
 * @details O MESMO firmware e gravado nas duas placas. Nao ha papel fixo:
 *          a placa em que o botao azul (B1) e pressionado vira o
 *          "modulo A" (transmissor) daquele teste; a outra atua como
 *          "modulo B" (receptor) e responde ACK/NACK automaticamente.
 *
 * | Botao B1                 | Teste executado pela placa A                             |
 * |--------------------------|----------------------------------------------------------|
 * | clique (< 1 s)           | Minimo: envia vetor de 100 bytes {0,1,...,99}, mede tempo |
 * | segurar 1 a 3 s (LED azul acende)     | Estresse: 1000 quadros de 100 bytes          |
 * | segurar > 3 s (LED vermelho acende)   | Estresse com 20% de quadros corrompidos de proposito |
 *
 * LEDs: verde = transmitindo; azul = sucesso; vermelho = falha.
 * Resultados detalhados saem no terminal serial (115200 8N1).
 *
 * @defgroup APP App - Aplicacao de teste
 * @{
 */
#ifndef APP_H
#define APP_H

/** @brief Inicializa o protocolo e imprime o cabecalho no console. */
void app_init(void);

/** @brief Uma iteracao do superloop (nao bloqueia, exceto durante testes). */
void app_loop(void);

#endif /* APP_H */
/** @} */
