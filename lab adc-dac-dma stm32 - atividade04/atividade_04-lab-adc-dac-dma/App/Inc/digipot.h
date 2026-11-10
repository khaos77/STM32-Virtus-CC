/**
 * @file    digipot.h
 * @brief   Potenciometro digital (digipot) feito com LED + LDR.
 *
 * @details O LED, acionado pelo DAC, ilumina o LDR. Quanto mais luz, menor a
 *          resistencia A-B do LDR. A palavra digital @c uint8_t @c res
 *          seleciona a resistencia atraves de uma look-up-table (LUT):
 *
 * @verbatim
 *   res (uint8) --> LUT[256] --> codigo DAC (12 bits) --> LED ~~luz~~> LDR (A-B)
 *
 *            3V3
 *             |
 *            LDR   <-- resistencia do digipot (R_LDR)
 *             |
 *             +-----> ADC1_IN3 (PA3)
 *             |
 *           R_serie (APP_R_SERIES_OHM)
 *             |
 *            GND
 * @endverbatim
 *
 *          Divisor de tensao e resistencia do LDR:
 *
 *          @f[ V_{ADC} = V_{CC}\,\frac{R_S}{R_S + R_{LDR}}
 *              \quad\Rightarrow\quad
 *              R_{LDR} = R_S\,\frac{V_{CC}-V_{ADC}}{V_{ADC}}
 *                      = R_S\,\frac{4095 - N_{ADC}}{N_{ADC}} @f]
 *
 *          Como ADC e divisor usam a mesma referencia (3,3 V), a medida e
 *          ratiometrica: nao depende do valor exato de @f$V_{CC}@f$.
 *
 *          A LUT calibrada lineariza o digipot:
 *
 *          @f[ R_{nominal}(res) = R_{min} + (R_{max}-R_{min})\,\frac{res}{255} @f]
 *
 *          A LUT fica em RAM e pode ser sobrescrita de tres formas:
 *          - digipot_lut_default(): rampa linear no DAC (nao calibrada);
 *          - digipot_autocal(): varredura do DAC medindo o LDR pelo ADC;
 *          - digipot_lut_load(): quadro binario recebido pela UART via DMA.
 *
 * @defgroup DIGIPOT Digipot (LED + LDR)
 * @ingroup  APP
 * @{
 */
#ifndef DIGIPOT_H
#define DIGIPOT_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Numero de entradas da LUT (uma por valor de @c uint8_t). */
#define DIGIPOT_LUT_SIZE        256U

/** @brief Resistencia devolvida quando o ADC le zero (circuito "aberto"). */
#define DIGIPOT_R_OPEN          0xFFFFFFFFUL

/** @name Quadro binario de recarga da LUT (little-endian)
 *
 * | Offset | Tamanho | Campo                                          |
 * |--------|---------|------------------------------------------------|
 * | 0      | 2       | magico @c 0xA5 @c 0x5A (sincronismo)           |
 * | 2      | 4       | @c r_min em ohms (@c uint32)                   |
 * | 6      | 4       | @c r_max em ohms (@c uint32)                   |
 * | 10     | 512     | 256 codigos do DAC (@c uint16, 0 a 4095)       |
 * | 522    | 2       | CRC16-CCITT-FALSE dos bytes 2 a 521 (@c uint16)|
 * @{
 */
#define DIGIPOT_FRAME_MAGIC0    0xA5U  /**< Primeiro byte de sincronismo          */
#define DIGIPOT_FRAME_MAGIC1    0x5AU  /**< Segundo byte de sincronismo           */
#define DIGIPOT_FRAME_BODY_SIZE (8U + 2U * DIGIPOT_LUT_SIZE) /**< r_min + r_max + LUT */
#define DIGIPOT_FRAME_DATA_SIZE (DIGIPOT_FRAME_BODY_SIZE + 2U) /**< Corpo + CRC   */
#define DIGIPOT_FRAME_SIZE      (DIGIPOT_FRAME_DATA_SIZE + 2U) /**< Com o magico  */
/** @} */

/** @brief Resultado das operacoes sobre a LUT. */
typedef enum
{
    DIGIPOT_OK = 0,        /**< Operacao concluida                                  */
    DIGIPOT_ERR_CRC,       /**< CRC do quadro recebido nao confere                  */
    DIGIPOT_ERR_VALUE,     /**< Codigo do DAC > 4095 ou r_min >= r_max              */
    DIGIPOT_ERR_CAL_POINTS /**< Autocalibracao: poucos pontos validos e monotonicos */
} digipot_status_t;

/** @brief Uma medida do divisor de tensao. */
typedef struct
{
    uint16_t adc_raw; /**< Codigo medio do ADC (0 a 4095)                        */
    uint32_t mv;      /**< Tensao no no do divisor (mV)                          */
    uint32_t r_ohm;   /**< Resistencia calculada do LDR (ohms)                   */
    bool     valid;   /**< @c true se o ADC esta dentro da faixa util configurada */
} digipot_meas_t;

/** @brief Carrega a LUT padrao e aplica @c res = 0. */
void digipot_init(void);

/**
 * @brief Seleciona a resistencia pela palavra digital.
 * @param res Indice da LUT: 0 = menor resistencia, 255 = maior.
 */
void digipot_set(uint8_t res);

/** @brief Ultima palavra digital aplicada com digipot_set(). */
uint8_t digipot_get(void);

/**
 * @brief Escreve um codigo diretamente no DAC, ignorando a LUT.
 * @param code Codigo de 12 bits (usado em calibracao e na malha de ohms).
 */
void digipot_set_dac(uint16_t code);

/** @brief Codigo atualmente aplicado ao DAC. */
uint16_t digipot_get_dac(void);

/**
 * @brief Mede a resistencia do LDR com media longa (bloqueia ~25 ms).
 * @param[out] m Resultado da medida.
 */
void digipot_measure(digipot_meas_t *m);

/**
 * @brief Mede a resistencia do LDR com a janela atual do ADC (nao bloqueia).
 * @param[out] m Resultado da medida.
 */
void digipot_measure_fast(digipot_meas_t *m);

/**
 * @brief Converte um codigo do ADC na resistencia do LDR.
 * @param raw Codigo de 12 bits lido no no do divisor.
 * @return Resistencia em ohms; @ref DIGIPOT_R_OPEN se @p raw for zero.
 */
uint32_t digipot_adc_to_ohms(uint16_t raw);

/** @brief @c true se a LUT atual veio de uma calibracao (autocal ou UART). */
bool digipot_is_calibrated(void);

/**
 * @brief Resistencia nominal associada a uma palavra digital.
 * @param res Palavra digital.
 * @return Resistencia em ohms, ou 0 se a LUT nao esta calibrada.
 */
uint32_t digipot_nominal_ohms(uint8_t res);

/**
 * @brief Palavra digital cuja resistencia nominal e a mais proxima do alvo.
 * @param r_ohm Resistencia desejada em ohms.
 * @return Palavra digital (0 se a LUT nao esta calibrada).
 */
uint8_t digipot_res_for_ohms(uint32_t r_ohm);

/** @brief Faixa nominal da LUT calibrada (0 e 0 se nao calibrada). */
void digipot_range(uint32_t *r_min, uint32_t *r_max);

/** @brief Acesso de leitura as @ref DIGIPOT_LUT_SIZE entradas da LUT. */
const uint16_t *digipot_lut(void);

/**
 * @brief Restaura a LUT padrao (rampa linear no DAC, nao calibrada).
 *
 * @f[ LUT[res] = 4095 - res\,\frac{4095 - DAC_{min}}{255} @f]
 */
void digipot_lut_default(void);

/**
 * @brief Sobrescreve a LUT com um quadro recebido pela UART.
 * @param data @ref DIGIPOT_FRAME_DATA_SIZE bytes: corpo + CRC, sem o magico.
 * @return @ref DIGIPOT_OK, @ref DIGIPOT_ERR_CRC ou @ref DIGIPOT_ERR_VALUE.
 * @note  A LUT anterior so e substituida se o quadro inteiro for valido.
 */
digipot_status_t digipot_lut_load(const uint8_t *data);

/**
 * @brief Constroi a LUT a partir de pontos (codigo DAC, resistencia) medidos.
 *
 * Para cada @c res calcula a resistencia nominal linear entre @p r_min e
 * @p r_max e interpola o codigo do DAC entre os dois pontos vizinhos da curva.
 * A interpolacao e feita na condutancia @f$G = 1/R@f$, que e aproximadamente
 * linear com a corrente do LED (logo, com o codigo do DAC):
 *
 * @f[ DAC = DAC_0 + (DAC_1 - DAC_0)\,
 *           \frac{1/R_{alvo} - 1/R_0}{1/R_1 - 1/R_0} @f]
 *
 * @param dac   Codigos do DAC, em ordem crescente.
 * @param r     Resistencias correspondentes, estritamente decrescentes.
 * @param n     Numero de pontos (>= 2).
 * @param r_min Resistencia associada a @c res = 0.
 * @param r_max Resistencia associada a @c res = 255.
 */
void digipot_build_lut(const uint16_t *dac, const uint32_t *r, uint32_t n,
                       uint32_t r_min, uint32_t r_max);

/**
 * @brief Autocalibracao: varre o DAC, mede o LDR pelo ADC e refaz a LUT.
 *
 * Bloqueia por ~@c APP_SETTLE_FIRST_MS + @c APP_CAL_POINTS x
 * (@c APP_SETTLE_MS + 25) ms (cerca de 19 s com os valores padrao).
 *
 * @param r_min_req Limite inferior desejado em ohms (0 = menor valor medido).
 * @param r_max_req Limite superior desejado em ohms (0 = maior valor medido).
 * @return @ref DIGIPOT_OK, @ref DIGIPOT_ERR_CAL_POINTS ou @ref DIGIPOT_ERR_VALUE.
 * @note  A varredura considera o resistor serie e o proprio divisor no
 *        circuito. Para calibrar contra o multimetro use o comando
 *        @c dsweep e a ferramenta @c tools/digipot_tool.py.
 */
digipot_status_t digipot_autocal(uint32_t r_min_req, uint32_t r_max_req);

/**
 * @brief CRC16-CCITT-FALSE (polinomio 0x1021, valor inicial 0xFFFF).
 * @param data Bytes de entrada.
 * @param len  Quantidade de bytes.
 * @return CRC de 16 bits ("123456789" -> 0x29B1).
 */
uint16_t digipot_crc16(const uint8_t *data, uint32_t len);

/** @brief Texto descritivo de um @ref digipot_status_t. */
const char *digipot_status_str(digipot_status_t st);

#endif /* DIGIPOT_H */
/** @} */
