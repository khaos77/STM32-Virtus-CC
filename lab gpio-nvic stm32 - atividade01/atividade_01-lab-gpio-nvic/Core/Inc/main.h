/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN_Pin GPIO_PIN_13
#define BTN_GPIO_Port GPIOC
#define BTN_EXTI_IRQn EXTI15_10_IRQn
#define LED_TX_Pin GPIO_PIN_0
#define LED_TX_GPIO_Port GPIOB
#define RX_CLK_Pin GPIO_PIN_14
#define RX_CLK_GPIO_Port GPIOF
#define RX_CLK_EXTI_IRQn EXTI15_10_IRQn
#define TX_CLK_Pin GPIO_PIN_15
#define TX_CLK_GPIO_Port GPIOF
#define RX_DATA_Pin GPIO_PIN_11
#define RX_DATA_GPIO_Port GPIOE
#define TX_DATA_Pin GPIO_PIN_13
#define TX_DATA_GPIO_Port GPIOE
#define LED_ERR_Pin GPIO_PIN_14
#define LED_ERR_GPIO_Port GPIOB
#define LED_OK_Pin GPIO_PIN_7
#define LED_OK_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
