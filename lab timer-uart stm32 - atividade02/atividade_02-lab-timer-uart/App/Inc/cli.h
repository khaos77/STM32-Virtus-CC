/**
 * @file    cli.h
 * @brief   Interface de linha de comando pela UART.
 *
 * @details Comandos (terminados por Enter, CR ou LF):
 *          | Comando            | Acao                                        |
 *          |--------------------|---------------------------------------------|
 *          | `60` ou `f 60`     | Define a frequencia da senoide em Hz        |
 *          | `on`               | Liga a saida                                |
 *          | `off`              | Desliga a saida                             |
 *          | `?`                | Mostra o estado atual e a ajuda             |
 *
 *          A frequencia aceita casas decimais com ponto ou virgula
 *          (ex.: `59.5` ou `59,5`).
 *
 * @defgroup CLI Linha de comando
 * @ingroup  APP
 * @{
 */
#ifndef CLI_H
#define CLI_H

/** @brief Tamanho maximo de uma linha de comando (inclui o terminador). */
#define CLI_LINE_MAX 32U

/** @brief Imprime o cabecalho com a ajuda e o estado atual. */
void cli_init(void);

/**
 * @brief Consome os caracteres recebidos e executa os comandos completos.
 * @note  Nao bloqueia: chamar continuamente no superloop.
 */
void cli_process(void);

#endif /* CLI_H */
/** @} */
