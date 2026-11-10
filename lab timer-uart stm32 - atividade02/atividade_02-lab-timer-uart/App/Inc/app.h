/**
 * @file    app.h
 * @brief   Aplicacao: gerador senoidal com frequencia ajustavel pela UART.
 * @ingroup APP
 */
#ifndef APP_H
#define APP_H

/**
 * @brief Inicializa o gerador, inicia a saida na frequencia padrao e
 *        apresenta a linha de comando.
 * @note  Requer bsp_init() ja executado.
 */
void app_init(void);

/**
 * @brief Corpo do superloop: trata comandos da UART e pisca o LED de status.
 * @note  Nao bloqueia.
 */
void app_loop(void);

#endif /* APP_H */
