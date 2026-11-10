/**
 * @file    app_config.h
 * @brief   Parametros de projeto do gerador senoidal.
 *
 * @details Filtro RC passa-baixas de 1a ordem na saida PA6:
 *          R = 1 kOhm, C = 100 nF  ->  fc = 1 / (2*pi*R*C) = 1591,5 Hz.
 *
 *          Limites de frequencia da senoide (onde o filtro continua efetivo):
 *          - Maximo 1 kHz: |H(1 kHz)| = 0,847 (-1,44 dB). Acima disso a
 *            fundamental comeca a ser atenuada demais e a razao
 *            portadora/fundamental cai abaixo de 100.
 *          - Minimo 1 Hz: limite pratico de observacao no osciloscopio
 *            (a resolucao do DDS e de 23 uHz, nao e o fator limitante).
 *          - Portadora (100 kHz): |H| = 0,0159 (-36 dB) em toda a faixa.
 *
 * @defgroup APP Aplicacao
 * @{
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/** @brief Menor frequencia aceita pela UART, em Hz. */
#define APP_SINE_FREQ_MIN_HZ      1.0f

/** @brief Maior frequencia aceita pela UART, em Hz. */
#define APP_SINE_FREQ_MAX_HZ      1000.0f

/** @brief Frequencia gerada apos o reset, em Hz. */
#define APP_SINE_FREQ_DEFAULT_HZ  60.0f

/**
 * @brief Indice de modulacao (0 a 1): fracao da excursao total do duty.
 * @details 0,90 mantem o duty entre 5 % e 95 %, evitando pulsos muito
 *          estreitos (que o pino nao reproduz bem e que distorcem os picos).
 */
#define APP_SINE_MOD_INDEX        0.90f

/** @brief Periodo do pisca do LED de status, em ms. */
#define APP_HEARTBEAT_MS          500U

/** @brief Tempo que o LED de erro fica aceso apos um comando invalido, em ms. */
#define APP_ERR_LED_MS            300U

#endif /* APP_CONFIG_H */
/** @} */
