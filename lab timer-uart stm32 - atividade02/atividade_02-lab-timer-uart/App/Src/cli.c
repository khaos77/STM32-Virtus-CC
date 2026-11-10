/**
 * @file    cli.c
 * @brief   Implementacao da linha de comando.
 * @ingroup CLI
 */
#include "cli.h"
#include "app_config.h"
#include "bsp.h"
#include "sine_gen.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char     s_line[CLI_LINE_MAX]; /**< Linha em montagem              */
static uint32_t s_len = 0U;           /**< Caracteres ja recebidos        */
static uint32_t s_err_until = 0U;     /**< Instante de apagar o LED de erro */
static bool     s_err_on = false;     /**< LED de erro aceso?             */

/**
 * @brief Imprime uma frequencia com 2 casas decimais sem usar @c %f
 *        (evita linkar o suporte a float do printf da newlib-nano).
 * @param hz Frequencia em Hz.
 */
static void print_hz(float hz)
{
    uint32_t centi = (uint32_t)(hz * 100.0f + 0.5f);
    printf("%lu.%02lu Hz", (unsigned long)(centi / 100U), (unsigned long)(centi % 100U));
}

/** @brief Imprime estado atual do gerador. */
static void print_status(void)
{
    printf("Saida: %s | f = ", sine_gen_is_running() ? "ON" : "OFF");
    print_hz(sine_gen_get_frequency());
    printf(" | portadora = %lu Hz\r\n", (unsigned long)bsp_pwm_get_carrier_hz());
}

/** @brief Imprime a lista de comandos e os limites de frequencia. */
static void print_help(void)
{
    printf("Comandos:\r\n"
           "  <freq> | f <freq>  frequencia em Hz (%d a %d)\r\n"
           "  on / off           liga / desliga a saida\r\n"
           "  ?                  estado e ajuda\r\n",
           (int)APP_SINE_FREQ_MIN_HZ, (int)APP_SINE_FREQ_MAX_HZ);
}

/** @brief Sinaliza comando invalido: mensagem + LED vermelho temporizado. */
static void report_error(const char *msg)
{
    printf("ERRO: %s\r\n", msg);
    bsp_led_set(BSP_LED_ERR, true);
    s_err_on    = true;
    s_err_until = bsp_millis() + APP_ERR_LED_MS;
}

/**
 * @brief Interpreta e executa uma linha completa.
 * @param line Linha terminada em NUL (e modificada: virgula vira ponto).
 */
static void execute(char *line)
{
    while (isspace((unsigned char)*line))
    {
        line++;
    }

    if (*line == '\0')
    {
        return; /* linha vazia */
    }
    if (strcmp(line, "?") == 0)
    {
        print_status();
        print_help();
        return;
    }
    if (strcmp(line, "on") == 0)
    {
        sine_gen_start();
        print_status();
        return;
    }
    if (strcmp(line, "off") == 0)
    {
        sine_gen_stop();
        print_status();
        return;
    }

    /* Frequencia: prefixo "f" opcional, virgula aceita como separador decimal */
    if ((*line == 'f') || (*line == 'F'))
    {
        line++;
    }
    for (char *p = line; *p != '\0'; p++)
    {
        if (*p == ',')
        {
            *p = '.';
        }
    }

    char *end;
    float hz = strtof(line, &end);
    while (isspace((unsigned char)*end))
    {
        end++;
    }

    if ((end == line) || (*end != '\0'))
    {
        report_error("comando invalido ('?' para ajuda)");
    }
    else if (!sine_gen_set_frequency(hz))
    {
        report_error("frequencia fora dos limites do filtro");
    }
    else
    {
        printf("OK: f = ");
        print_hz(sine_gen_get_frequency());
        printf("\r\n");
    }
}

void cli_init(void)
{
    printf("\r\n=== Gerador senoidal PWM + RC (Lab Timer/UART) ===\r\n");
    print_status();
    print_help();
    printf("> ");
}

void cli_process(void)
{
    char ch;

    while (bsp_console_read_char(&ch))
    {
        if ((ch == '\r') || (ch == '\n'))
        {
            if (s_len > 0U) /* ignora o LF de um par CR+LF */
            {
                printf("\r\n");
                s_line[s_len] = '\0';
                execute(s_line);
                s_len = 0U;
                printf("> ");
            }
        }
        else if ((ch == '\b') || (ch == 0x7F)) /* backspace / delete */
        {
            if (s_len > 0U)
            {
                s_len--;
                printf("\b \b");
            }
        }
        else if (isprint((unsigned char)ch) && (s_len < (CLI_LINE_MAX - 1U)))
        {
            s_line[s_len++] = ch;
            putchar(ch); /* eco */
        }
    }

    /* Apaga o LED de erro apos o tempo configurado (subtracao a prova de estouro) */
    if (s_err_on && ((int32_t)(bsp_millis() - s_err_until) >= 0))
    {
        bsp_led_set(BSP_LED_ERR, false);
        s_err_on = false;
    }
}
