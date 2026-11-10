/**
 * @file    app_config.h
 * @brief   Parametros de ajuste do digipot (montagem, tempos e ganhos).
 * @ingroup APP
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/**
 * @brief Resistor em serie com o LDR (entre o no do ADC e o GND), em ohms.
 * @note  Ajuste para o valor REAL medido com o multimetro: ele entra
 *        diretamente no calculo da resistencia do LDR.
 */
#define APP_R_SERIES_OHM          10000U

/**
 * @brief Codigo do DAC logo abaixo do limiar de conducao do LED.
 *
 * 1700 -> ~1,37 V (LED vermelho conduz a partir de ~1,6 V). Abaixo disso o
 * LED fica apagado e os codigos seriam desperdicados.
 */
#define APP_DAC_LED_MIN_CODE      1700U

/**
 * @brief Menor codigo do ADC aceito como medida valida.
 *
 * Com o LDR no escuro a tensao no divisor tende a zero e a resolucao em
 * ohms desaba: 150 contagens equivalem a ~26 x R_serie.
 */
#define APP_ADC_MIN_VALID         150U

/** @brief Maior codigo do ADC aceito como medida valida (~R_serie / 26). */
#define APP_ADC_MAX_VALID         3945U

/** @brief Janelas de 64 amostras promediadas em cada medida "lenta". */
#define APP_MEAS_WINDOWS          8U

/** @brief Tempo de acomodacao do LDR apos cada passo de luz (ms). */
#define APP_SETTLE_MS             250U

/** @brief Acomodacao antes do primeiro ponto de uma varredura (ms). */
#define APP_SETTLE_FIRST_MS       1500U

/** @brief Numero de pontos da varredura de autocalibracao (comando @c cal). */
#define APP_CAL_POINTS            64U

/** @brief Tempo maximo para receber o quadro da LUT apos o comando @c load (ms). */
#define APP_LOAD_TIMEOUT_MS       5000U

/** @brief Periodo do controlador da aplicacao exemplo (comando @c ohm), em ms. */
#define APP_OHM_PERIOD_MS         100U

/**
 * @brief Ganho integral da aplicacao exemplo: variacao do DAC (em codigos)
 *        por periodo para um erro relativo de 100 %.
 */
#define APP_OHM_GAIN_CODES        60

/** @brief Erro relativo (em partes por mil) abaixo do qual a malha esta "travada". */
#define APP_OHM_LOCK_PERMILLE     20

/** @brief Intervalo entre relatorios da malha no console (ms). */
#define APP_OHM_REPORT_MS         500U

#endif /* APP_CONFIG_H */
