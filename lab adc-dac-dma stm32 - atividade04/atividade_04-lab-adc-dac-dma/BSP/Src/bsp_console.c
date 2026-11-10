/**
 * @file    bsp_console.c
 * @brief   Console: retarget de printf e recepcao por DMA circular.
 * @ingroup BSP_CONSOLE
 */
#include "bsp_console.h"
#include <stdio.h>

#define RX_MASK (BSP_CONSOLE_RX_BUF_SIZE - 1U) /**< Mascara de indice do buffer */

static UART_HandleTypeDef *s_huart = NULL;                 /**< UART do console (ou NULL)   */
static uint8_t             s_rx_buf[BSP_CONSOLE_RX_BUF_SIZE]; /**< Buffer escrito pelo DMA  */
static uint32_t            s_rx_tail = 0U;                 /**< Proxima posicao a ler       */
static volatile uint32_t   s_rx_restarts = 0U;             /**< Reinicios por erro de linha */

/** @brief Inicia (ou reinicia) a recepcao por DMA circular a partir do indice 0. */
static bool rx_start(void)
{
    s_rx_tail = 0U;
    return HAL_UART_Receive_DMA(s_huart, s_rx_buf,
                                (uint16_t)BSP_CONSOLE_RX_BUF_SIZE) == HAL_OK;
}

/** @brief Posicao em que o DMA escrevera o proximo byte. */
static uint32_t rx_head(void)
{
    return (BSP_CONSOLE_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(s_huart->hdmarx)) & RX_MASK;
}

void bsp_console_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (s_huart != NULL)
    {
        (void)rx_start();           /* se falhar, bsp_console_available() religa */
    }
}

uint32_t bsp_console_available(void)
{
    if (s_huart == NULL)
    {
        return 0U;
    }

    /* Apos um erro de linha a HAL aborta a recepcao: religa aqui. */
    if (s_huart->RxState == HAL_UART_STATE_READY)
    {
        s_rx_restarts++;
        (void)rx_start();
        return 0U;                  /* buffer recomeca vazio */
    }
    return (rx_head() - s_rx_tail) & RX_MASK;
}

bool bsp_console_read_byte(uint8_t *byte)
{
    if (bsp_console_available() == 0U)
    {
        return false;
    }
    *byte     = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1U) & RX_MASK;
    return true;
}

void bsp_console_flush_rx(void)
{
    if (s_huart != NULL)
    {
        s_rx_tail = rx_head();
    }
}

uint32_t bsp_console_rx_restarts(void)
{
    return s_rx_restarts;
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
        (void)HAL_UART_Transmit(s_huart, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    }
    return len;
}
