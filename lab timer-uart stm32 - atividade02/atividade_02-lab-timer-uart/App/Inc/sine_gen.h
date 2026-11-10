/**
 * @file    sine_gen.h
 * @brief   Gerador senoidal por PWM usando DDS (sintese digital direta).
 *
 * @details A cada periodo de PWM (fs = 100 kHz) um acumulador de fase de
 *          32 bits e incrementado e seus 10 bits mais significativos indexam
 *          uma tabela de 1024 amostras de seno ja convertidas em valor de
 *          compare do timer:
 *
 *          @code
 *          fase  += incremento            (a cada update do timer)
 *          CCR1   = tabela[fase >> 22]
 *          incremento = f_out * 2^32 / fs
 *          @endcode
 *
 *          Diferente da abordagem do livro (tabela fixa percorrida uma amostra
 *          por periodo, com f_out alterada mudando a frequencia do PWM), aqui
 *          a portadora e fixa. Assim o filtro RC e projetado uma unica vez e a
 *          frequencia pode ser ajustada com resolucao de fs/2^32 = 23 uHz,
 *          sem descontinuidade de fase.
 *
 * @defgroup SINE_GEN Gerador senoidal
 * @ingroup  APP
 * @{
 */
#ifndef SINE_GEN_H
#define SINE_GEN_H

#include <stdbool.h>
#include <stdint.h>

/** @brief log2 do numero de amostras da tabela de seno. */
#define SINE_GEN_LUT_BITS 10U

/** @brief Numero de amostras da tabela de seno (um periodo completo). */
#define SINE_GEN_LUT_SIZE (1UL << SINE_GEN_LUT_BITS)

/**
 * @brief Calcula a tabela de seno a partir da resolucao do PWM e programa a
 *        frequencia padrao (APP_SINE_FREQ_DEFAULT_HZ). Nao inicia a saida.
 * @note  Requer o BSP ja inicializado.
 */
void sine_gen_init(void);

/** @brief Inicia a geracao (PWM + interrupcao de update). */
void sine_gen_start(void);

/** @brief Para a geracao (saida em nivel baixo). */
void sine_gen_stop(void);

/**
 * @brief Indica se a saida esta ativa.
 * @return @c true se o PWM esta rodando.
 */
bool sine_gen_is_running(void);

/**
 * @brief Altera a frequencia da senoide, sem salto de fase.
 * @param freq_hz Frequencia desejada em Hz.
 * @return @c true se aceita; @c false se fora de
 *         [APP_SINE_FREQ_MIN_HZ, APP_SINE_FREQ_MAX_HZ] (nada muda).
 */
bool sine_gen_set_frequency(float freq_hz);

/**
 * @brief Frequencia atualmente programada.
 * @return Frequencia em Hz.
 */
float sine_gen_get_frequency(void);

#endif /* SINE_GEN_H */
/** @} */
