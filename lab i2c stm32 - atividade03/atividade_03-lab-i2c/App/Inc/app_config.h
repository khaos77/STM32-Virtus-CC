/**
 * @file    app_config.h
 * @brief   Parametros de ajuste da aplicacao (periodos e ganho do filtro).
 * @ingroup APP
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/** @brief Periodo nominal de amostragem do MPU6050 (us). 5000 us = 200 Hz. */
#define APP_SAMPLE_PERIOD_US     5000U

/**
 * @brief Ganho @f$\alpha@f$ do filtro complementar (peso do giroscopio).
 *
 * Constante de tempo equivalente:
 * @f$ \tau = \frac{\alpha\,T_s}{1-\alpha} = \frac{0{,}98 \cdot 5\,ms}{0{,}02} \approx 0{,}25\,s @f$
 */
#define APP_FILTER_ALPHA         0.98f

/** @brief Intervalo minimo entre dois quadros do display (ms). */
#define APP_DISPLAY_PERIOD_MS    50U

/** @brief Intervalo entre linhas de log no console serial (ms). */
#define APP_LOG_PERIOD_MS        250U

/** @brief Numero de amostras usadas na calibracao do bias do giroscopio. */
#define APP_GYRO_CALIB_SAMPLES   400U

/** @brief Falhas consecutivas de leitura que disparam a reinicializacao do sensor. */
#define APP_MAX_READ_ERRORS      20U

/** @brief Intervalo entre tentativas de (re)inicializar os dispositivos I2C (ms). */
#define APP_RETRY_PERIOD_MS      500U

#endif /* APP_CONFIG_H */
