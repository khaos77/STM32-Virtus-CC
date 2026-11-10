/**
 * @file    bsp_console.c
 * @brief   Console: retarget de printf e recepcao por interrupcao.
 * @ingroup BSP_CONSOLE
 */
#include "bsp_console.h"
#include <stdio.h>

#define RX_MASK (BSP_CONSOLE_RX_BUF_SIZE - 1U) /**< Mascara de indice do buffer */

static UART_HandleTypeDef *s_huart = NULL;            /**< UART do console (ou NULL) */
static uint8_t           s_rx_byte;                   /**< Byte em recepcao pela HAL */
static volatile uint8_t  s_rx_buf[BSP_CONSOLE_RX_BUF_SIZE]; /**< Buffer circular   */
static volatile uint32_t s_rx_head = 0U;              /**< Escrita (ISR)             */
static volatile uint32_t s_rx_tail = 0U;              /**< Leitura (superloop)       */

void bsp_console_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (s_huart != NULL)
    {
        HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U);
    }
}

bool bsp_console_read_char(char *ch)
{
    if (s_rx_head == s_rx_tail)
    {
        return false;
    }
    *ch = (char)s_rx_buf[s_rx_tail & RX_MASK];
    s_rx_tail++;
    return true;
}

/**
 * @brief Callback da HAL: um byte chegou. Guarda no buffer circular e rearma.
 * @param huart UART que recebeu.
 * @note  Se o buffer estiver cheio, o byte e descartado.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == s_huart)
    {
        if ((s_rx_head - s_rx_tail) < BSP_CONSOLE_RX_BUF_SIZE)
        {
            s_rx_buf[s_rx_head & RX_MASK] = s_rx_byte;
            s_rx_head++;
        }
        HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U);
    }
}

/**
 * @brief Callback da HAL: erro de recepcao (overrun, ruido, framing).
 *        A HAL aborta a recepcao; aqui ela e rearmada.
 * @param huart UART com erro.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == s_huart)
    {
        HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1U);
    }
}

/**
 * @brief Implementacao forte de @c _write usada pela newlib (printf, puts...).
 * @return Quantidade de bytes "escritos" (sempre @p len).
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if ((s_huart != NULL) && (len > 0))
    {
        HAL_UART_Transmit(s_huart, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    }
    return len;
}
