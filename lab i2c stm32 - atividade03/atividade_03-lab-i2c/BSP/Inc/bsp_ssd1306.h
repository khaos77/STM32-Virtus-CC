/**
 * @file    bsp_ssd1306.h
 * @brief   Display OLED SSD1306 128x64 monocromatico via I2C.
 *
 * @details O driver mantem um framebuffer de 1024 bytes em RAM (1 bit por
 *          pixel, organizado em 8 paginas de 128 colunas, como na memoria do
 *          controlador). As funcoes de desenho so alteram o framebuffer; o
 *          envio ao display e feito por:
 *
 *          - @ref bsp_ssd1306_update : envia o quadro inteiro (bloqueia ~25 ms);
 *          - @ref bsp_ssd1306_update_step : envia UMA pagina por chamada
 *            (~3 ms), permitindo intercalar a atualizacao da tela com a
 *            amostragem do MPU6050 no superloop.
 *
 *          Sistema de coordenadas: (0,0) no canto superior esquerdo, x cresce
 *          para a direita e y para baixo.
 *
 * @defgroup BSP_SSD1306 SSD1306
 * @ingroup  BSP
 * @{
 */
#ifndef BSP_SSD1306_H
#define BSP_SSD1306_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Endereco I2C de 7 bits (alguns modulos usam 0x3D). */
#define BSP_SSD1306_ADDR      0x3CU
/** @brief Largura do display em pixels. */
#define BSP_SSD1306_WIDTH     128
/** @brief Altura do display em pixels. */
#define BSP_SSD1306_HEIGHT    64
/** @brief Deslocamento de coluna (0 para SSD1306; 2 para modulos SH1106 de 1,3"). */
#define BSP_SSD1306_X_OFFSET  0U

/** @brief Largura de uma celula de caractere (5 colunas do glifo + 1 de espaco). */
#define BSP_SSD1306_CHAR_W    6
/** @brief Altura de uma celula de caractere. */
#define BSP_SSD1306_CHAR_H    8
/** @brief Caractere extra da fonte: simbolo de grau. */
#define BSP_SSD1306_CHAR_DEG  '\x7F'

/**
 * @brief Envia a sequencia de inicializacao e apaga a tela.
 * @return @c true se o display respondeu no barramento.
 */
bool bsp_ssd1306_init(void);

/** @brief Apaga o framebuffer (nao envia ao display). */
void bsp_ssd1306_clear(void);

/**
 * @brief Acende ou apaga um pixel do framebuffer.
 * @param x  Coluna (fora da tela e ignorado).
 * @param y  Linha (fora da tela e ignorado).
 * @param on @c true acende, @c false apaga.
 */
void bsp_ssd1306_pixel(int x, int y, bool on);

/** @brief Desenha uma reta entre dois pontos (algoritmo de Bresenham). */
void bsp_ssd1306_line(int x0, int y0, int x1, int y1);

/** @brief Desenha uma circunferencia (algoritmo do ponto medio). */
void bsp_ssd1306_circle(int cx, int cy, int r);

/**
 * @brief Desenha um caractere 5x7 (celula 6x8, fundo apagado).
 * @param x Coluna do canto superior esquerdo, em pixels.
 * @param y Linha do canto superior esquerdo, em pixels.
 * @param c Caractere ASCII 0x20..0x7E ou @ref BSP_SSD1306_CHAR_DEG.
 */
void bsp_ssd1306_char(int x, int y, char c);

/** @brief Desenha uma string terminada em NUL a partir de (x, y). */
void bsp_ssd1306_string(int x, int y, const char *s);

/**
 * @brief Envia o framebuffer inteiro ao display (bloqueante).
 * @return @c true se todas as transacoes I2C tiveram sucesso.
 */
bool bsp_ssd1306_update(void);

/**
 * @brief Envia a proxima pagina (8 linhas) do framebuffer ao display.
 * @return @c true quando a ultima pagina do quadro acabou de ser enviada.
 */
bool bsp_ssd1306_update_step(void);

#endif /* BSP_SSD1306_H */
/** @} */
