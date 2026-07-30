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
#include "stm32f4xx_hal.h"

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
#define in3_Pin GPIO_PIN_2
#define in3_GPIO_Port GPIOE
#define in4_Pin GPIO_PIN_3
#define in4_GPIO_Port GPIOE
#define in5_Pin GPIO_PIN_4
#define in5_GPIO_Port GPIOE
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOC
#define pin_5v_en_Pin GPIO_PIN_0
#define pin_5v_en_GPIO_Port GPIOD
#define pin_3v3_en_Pin GPIO_PIN_1
#define pin_3v3_en_GPIO_Port GPIOD
#define out1_Pin GPIO_PIN_3
#define out1_GPIO_Port GPIOD
#define out2_Pin GPIO_PIN_4
#define out2_GPIO_Port GPIOD
#define out3_Pin GPIO_PIN_5
#define out3_GPIO_Port GPIOD
#define out4_Pin GPIO_PIN_6
#define out4_GPIO_Port GPIOD
#define out5_Pin GPIO_PIN_7
#define out5_GPIO_Port GPIOD
#define in1_Pin GPIO_PIN_0
#define in1_GPIO_Port GPIOE
#define in2_Pin GPIO_PIN_1
#define in2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
