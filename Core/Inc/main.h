/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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
void SystemClock_Config(void);
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STATUS1_Pin GPIO_PIN_2
#define STATUS1_GPIO_Port GPIOE
#define STATUS2_Pin GPIO_PIN_3
#define STATUS2_GPIO_Port GPIOE
#define STATUS3_Pin GPIO_PIN_4
#define STATUS3_GPIO_Port GPIOE
#define ERROR0_Pin GPIO_PIN_5
#define ERROR0_GPIO_Port GPIOE
#define ERROR1_Pin GPIO_PIN_6
#define ERROR1_GPIO_Port GPIOE
#define Payload_Fault_VBatt_Pin GPIO_PIN_2
#define Payload_Fault_VBatt_GPIO_Port GPIOC
#define Payload_EN_VBatt_Pin GPIO_PIN_3
#define Payload_EN_VBatt_GPIO_Port GPIOC
#define Payload_Fault_12V_Pin GPIO_PIN_2
#define Payload_Fault_12V_GPIO_Port GPIOA
#define Payload_EN_12V_Pin GPIO_PIN_3
#define Payload_EN_12V_GPIO_Port GPIOA
#define Payload_EN_5V_Pin GPIO_PIN_4
#define Payload_EN_5V_GPIO_Port GPIOC
#define Payload_Port_Det_Pin GPIO_PIN_5
#define Payload_Port_Det_GPIO_Port GPIOC
#define Payload_Fault_5V_Pin GPIO_PIN_12
#define Payload_Fault_5V_GPIO_Port GPIOE
#define SPI_CS8_Pin GPIO_PIN_9
#define SPI_CS8_GPIO_Port GPIOD
#define SPI_CS3_Pin GPIO_PIN_10
#define SPI_CS3_GPIO_Port GPIOD
#define TailLight_EN_Pin GPIO_PIN_15
#define TailLight_EN_GPIO_Port GPIOA
#define SPI_CS2_Pin GPIO_PIN_0
#define SPI_CS2_GPIO_Port GPIOD
#define SPI_CS1_Pin GPIO_PIN_1
#define SPI_CS1_GPIO_Port GPIOD
#define SPI_CS5_Pin GPIO_PIN_3
#define SPI_CS5_GPIO_Port GPIOD
#define SPI_CS6_Pin GPIO_PIN_4
#define SPI_CS6_GPIO_Port GPIOD
#define SPI_CS4_Pin GPIO_PIN_7
#define SPI_CS4_GPIO_Port GPIOD
#define SPI_CS7_Pin GPIO_PIN_3
#define SPI_CS7_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
