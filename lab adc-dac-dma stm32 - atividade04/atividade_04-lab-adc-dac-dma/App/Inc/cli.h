/**
 * @file    cli.h
 * @brief   Linha de comando do digipot pelo console serial (115200 8N1).
 *
 * @details Os bytes chegam pelo buffer circular de DMA da BSP. A CLI tem dois
 *          estados:
 *
 * @verbatim
 *                 "load" + Enter
 *   +---------+ ------------------> +------------------------------+
 *   |  TEXTO  |                     |  CARGA (quadro binario)      |
 *   | comandos| <------------------ |  A5 5A + 520 bytes + CRC16   |
 *   +---------+   quadro completo   +------------------------------+
 *                 ou timeout de 5 s
 * @endverbatim
 *
 * | Comando                | Acao                                                         |
 * |------------------------|--------------------------------------------------------------|
 * | @c help                | lista os comandos                                            |
 * | @c info                | estado: res, DAC, faixa calibrada                            |
 * | @c res @c n            | aplica a palavra digital (0 a 255) pela LUT                  |
 * | @c dac @c n            | escreve o codigo bruto do DAC (0 a 4095), sem LUT            |
 * | @c read                | mede ADC, tensao e resistencia do LDR                        |
 * | @c sweep @c [passo]    | curva "Resistencia x res" em CSV                             |
 * | @c dsweep @c [pontos]  | curva "Resistencia x codigo DAC" em CSV (para calibrar)      |
 * | @c cal @c [rmin rmax]  | autocalibracao: refaz a LUT medindo o LDR pelo ADC           |
 * | @c load                | recebe uma nova LUT pela UART (DMA) - quadro binario         |
 * | @c default             | restaura a LUT padrao (nao calibrada)                        |
 * | @c lut                 | imprime a LUT atual                                          |
 * | @c test                | roteiro de testes: 10 valores de res e RMSE                  |
 * | @c ohm @c r            | aplicacao exemplo: mantem r ohms em malha fechada            |
 * | @c stop                | desliga a malha do comando @c ohm                            |
 *
 * @defgroup CLI Linha de comando
 * @ingroup  APP
 * @{
 */
#ifndef CLI_H
#define CLI_H

/** @brief Zera o estado da CLI e mostra o prompt. */
void cli_init(void);

/** @brief Consome os bytes recebidos; chamar a cada iteracao do superloop. */
void cli_task(void);

#endif /* CLI_H */
/** @} */
