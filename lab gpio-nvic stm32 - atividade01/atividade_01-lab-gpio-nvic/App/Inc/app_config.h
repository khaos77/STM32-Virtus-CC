/**
 * @file    app_config.h
 * @brief   Parametros dos testes da aplicacao.
 * @ingroup APP
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/** Tamanho do vetor do teste minimo (100 inteiros de 8 bits). */
#define APP_TEST_LEN              100u

/** Quantidade de quadros nos testes de estresse. */
#define APP_STRESS_FRAMES         1000u

/** Probabilidade (%) de corromper um quadro no teste com injecao de falhas. */
#define APP_STRESS_FAULT_PERCENT  20u

/** Segurar o botao por mais que isso -> teste de estresse. */
#define APP_LONG_PRESS_MS         1000u

/** Segurar o botao por mais que isso -> estresse com injecao de falhas. */
#define APP_VERY_LONG_PRESS_MS    3000u

/** Sem quadros por este tempo = fim de uma rajada no receptor (imprime resumo). */
#define APP_RX_BURST_END_MS       300u

/** Passo do padrao de dados do teste de estresse (ver app.c). */
#define APP_STRESS_PATTERN_STEP   13u

#endif /* APP_CONFIG_H */
