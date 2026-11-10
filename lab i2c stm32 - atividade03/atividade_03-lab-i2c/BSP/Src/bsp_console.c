/**
 * @file    bsp_console.c
 * @brief   Retarget de printf para a UART de log.
 * @ingroup BSP_CONSOLE
 */
#include "bsp_console.h"
#include <stdio.h>

static UART_HandleTypeDef *s_huart = NULL; /**< UART de log (ou NULL) */

void bsp_console_init(UART_HandleTypeDef *huart)
{
    s_huart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
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
