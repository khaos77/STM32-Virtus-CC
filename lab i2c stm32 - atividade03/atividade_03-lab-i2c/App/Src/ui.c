/**
 * @file    ui.c
 * @brief   Desenho das telas de dados e de animacao.
 * @ingroup UI
 */
#include "ui.h"
#include "bsp_ssd1306.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEG2RAD   0.01745329252f

/** @name Geometria da tela de animacao
 *  @{ */
#define HZ_CX     31      /**< Centro X do horizonte artificial */
#define HZ_CY     32      /**< Centro Y do horizonte artificial */
#define HZ_R      30      /**< Raio do horizonte artificial     */
#define CP_CX     97      /**< Centro X da bussola              */
#define CP_CY     24      /**< Centro Y da bussola              */
#define CP_R      17      /**< Raio da bussola                  */
/** @} */

void ui_format_fixed(char *buf, float value, uint8_t decimals, uint8_t width)
{
    static const long pow10[4] = { 1L, 10L, 100L, 1000L };
    char tmp[24];

    if (decimals > 3U) { decimals = 3U; }
    if (width > 15U)   { width = 15U; }

    long scale = pow10[decimals];
    long n     = lroundf(fabsf(value) * (float)scale);
    char sign  = ((value < 0.0f) && (n != 0L)) ? '-' : '+';

    if (decimals == 0U)
    {
        snprintf(tmp, sizeof(tmp), "%c%ld", sign, n);
    }
    else
    {
        snprintf(tmp, sizeof(tmp), "%c%ld.%0*ld", sign, n / scale,
                 (int)decimals, n % scale);
    }

    /* alinha a direita em um campo de 'width' caracteres */
    size_t len = strlen(tmp);
    size_t pad = (len < width) ? ((size_t)width - len) : 0U;
    memset(buf, ' ', pad);
    memcpy(&buf[pad], tmp, (size_t)width - pad);
    buf[width] = '\0';
}

/** @brief Escreve uma string centralizada horizontalmente na linha @p y. */
static void draw_centered(int y, const char *s)
{
    int w = (int)strlen(s) * BSP_SSD1306_CHAR_W;
    bsp_ssd1306_string((BSP_SSD1306_WIDTH - w) / 2, y, s);
}

void ui_draw_message(const char *l1, const char *l2, const char *l3)
{
    bsp_ssd1306_clear();
    if (l1 != NULL) { draw_centered(12, l1); }
    if (l2 != NULL) { draw_centered(28, l2); }
    if (l3 != NULL) { draw_centered(44, l3); }
}

void ui_draw_data(const bsp_mpu6050_data_t *imu, const attitude_t *att)
{
    static const char axis[3] = { 'X', 'Y', 'Z' };
    char num[16];

    bsp_ssd1306_clear();
    bsp_ssd1306_string(0, 0, " ACEL[m/s2] GIRO[d/s]");

    for (int i = 0; i < 3; i++)
    {
        int y = 8 * (i + 1);
        bsp_ssd1306_char(0, y, axis[i]);
        ui_format_fixed(num, imu->accel_ms2[i], 2, 7);      /* ex.: " -19.61" */
        bsp_ssd1306_string(2 * BSP_SSD1306_CHAR_W, y, num);
        ui_format_fixed(num, imu->gyro_dps[i], 1, 7);       /* ex.: " -500.0" */
        bsp_ssd1306_string(13 * BSP_SSD1306_CHAR_W, y, num);
    }

    bsp_ssd1306_line(0, 35, BSP_SSD1306_WIDTH - 1, 35);     /* separador */

    const struct { const char *name; float value; } ang[3] = {
        { "ROLL",  att->roll  },
        { "PITCH", att->pitch },
        { "YAW",   att->yaw   },
    };
    for (int i = 0; i < 3; i++)
    {
        int y = 40 + 8 * i;
        bsp_ssd1306_string(0, y, ang[i].name);
        ui_format_fixed(num, ang[i].value, 1, 7);           /* ex.: " -179.9" */
        bsp_ssd1306_string(6 * BSP_SSD1306_CHAR_W, y, num);
        bsp_ssd1306_char(13 * BSP_SSD1306_CHAR_W, y, BSP_SSD1306_CHAR_DEG);
    }
}

/**
 * @brief Horizonte artificial: o disco e dividido pela linha do horizonte;
 *        a metade "terra" e pontilhada e a metade "ceu" fica apagada.
 *
 * Visto da cabine, rolar para a direita faz o horizonte girar para a
 * esquerda, e baixar o nariz (pitch positivo) faz o horizonte subir. Para
 * cada pixel do disco calcula-se a distancia com sinal ate a reta do horizonte:
 * @f[ d = (x - x_0)\sin\phi + (y - y_0)\cos\phi, \qquad
 *     (x_0, y_0) = (c_x, c_y) - k\,\theta\,(\sin\phi, \cos\phi) @f]
 * @f$d > 0@f$ e terra, @f$d \approx 0@f$ e a propria linha do horizonte.
 */
static void draw_horizon(float roll_deg, float pitch_deg)
{
    const float s = sinf(roll_deg * DEG2RAD);
    const float c = cosf(roll_deg * DEG2RAD);
    /* pitch positivo (regra da mao direita em Y) = nariz para BAIXO,
     * portanto o horizonte sobe na tela (y diminui); 90 graus = raio */
    const float shift = -pitch_deg * ((float)HZ_R / 90.0f);
    const float x0 = (float)HZ_CX + shift * s;
    const float y0 = (float)HZ_CY + shift * c;

    for (int y = HZ_CY - HZ_R; y <= HZ_CY + HZ_R; y++)
    {
        for (int x = HZ_CX - HZ_R; x <= HZ_CX + HZ_R; x++)
        {
            int dx = x - HZ_CX;
            int dy = y - HZ_CY;
            if ((dx * dx + dy * dy) >= (HZ_R * HZ_R))
            {
                continue;                               /* fora do disco */
            }
            float d = ((float)x - x0) * s + ((float)y - y0) * c;
            if ((d >= -0.5f) && (d < 0.7f))
            {
                bsp_ssd1306_pixel(x, y, true);          /* linha do horizonte */
            }
            else if ((d > 0.0f) && (((x + 2 * y) % 4) == 0))
            {
                bsp_ssd1306_pixel(x, y, true);          /* terra pontilhada */
            }
        }
    }

    bsp_ssd1306_circle(HZ_CX, HZ_CY, HZ_R);

    /* simbolo fixo da aeronave: asas + ponto central */
    bsp_ssd1306_line(HZ_CX - 12, HZ_CY, HZ_CX - 4, HZ_CY);
    bsp_ssd1306_line(HZ_CX + 4,  HZ_CY, HZ_CX + 12, HZ_CY);
    bsp_ssd1306_line(HZ_CX - 4,  HZ_CY, HZ_CX - 4, HZ_CY + 3);
    bsp_ssd1306_line(HZ_CX + 4,  HZ_CY, HZ_CX + 4, HZ_CY + 3);
    bsp_ssd1306_pixel(HZ_CX, HZ_CY, true);
}

/**
 * @brief Bussola: ponteiro que parte do centro e aponta o yaw
 *        (0 graus = para cima; yaw positivo = anti-horario, visto de cima).
 */
static void draw_compass(float yaw_deg)
{
    const float s = sinf(yaw_deg * DEG2RAD);
    const float c = cosf(yaw_deg * DEG2RAD);

    bsp_ssd1306_circle(CP_CX, CP_CY, CP_R);
    bsp_ssd1306_line(CP_CX, CP_CY - CP_R, CP_CX, CP_CY - CP_R + 3); /* marca 0   */
    bsp_ssd1306_line(CP_CX - CP_R, CP_CY, CP_CX - CP_R + 2, CP_CY); /* marca +90 */
    bsp_ssd1306_line(CP_CX + CP_R, CP_CY, CP_CX + CP_R - 2, CP_CY); /* marca -90 */
    bsp_ssd1306_line(CP_CX, CP_CY + CP_R, CP_CX, CP_CY + CP_R - 2); /* marca 180 */

    const float len = (float)(CP_R - 4);
    int tip_x  = CP_CX - (int)lroundf(len * s);
    int tip_y  = CP_CY - (int)lroundf(len * c);
    int tail_x = CP_CX + (int)lroundf(0.4f * len * s);
    int tail_y = CP_CY + (int)lroundf(0.4f * len * c);
    bsp_ssd1306_line(tail_x, tail_y, tip_x, tip_y);

    /* ponta de seta */
    int lx = tip_x + (int)lroundf(4.0f * (s * 0.8f - c * 0.6f));
    int ly = tip_y + (int)lroundf(4.0f * (c * 0.8f + s * 0.6f));
    int rx = tip_x + (int)lroundf(4.0f * (s * 0.8f + c * 0.6f));
    int ry = tip_y + (int)lroundf(4.0f * (c * 0.8f - s * 0.6f));
    bsp_ssd1306_line(tip_x, tip_y, lx, ly);
    bsp_ssd1306_line(tip_x, tip_y, rx, ry);
}

void ui_draw_anim(const attitude_t *att)
{
    char num[16];
    const int tx = 66;                       /* coluna de texto a direita */
    const int cw = BSP_SSD1306_CHAR_W;

    bsp_ssd1306_clear();
    draw_horizon(att->roll, att->pitch);
    draw_compass(att->yaw);

    /* linha 1: "R+179 P+90" */
    bsp_ssd1306_char(tx, 46, 'R');
    ui_format_fixed(num, att->roll, 0, 4);
    bsp_ssd1306_string(tx + 1 * cw, 46, num);
    bsp_ssd1306_char(tx + 6 * cw, 46, 'P');
    ui_format_fixed(num, att->pitch, 0, 3);
    bsp_ssd1306_string(tx + 7 * cw, 46, num);

    /* linha 2: "YAW +179°" */
    bsp_ssd1306_string(tx, 56, "YAW");
    ui_format_fixed(num, att->yaw, 0, 4);
    bsp_ssd1306_string(tx + 4 * cw, 56, num);
    bsp_ssd1306_char(tx + 8 * cw, 56, BSP_SSD1306_CHAR_DEG);
}
