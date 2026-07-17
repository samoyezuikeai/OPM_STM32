/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_1
#define LED_GPIO_Port GPIOC
#define IN1_Pin GPIO_PIN_0
#define IN1_GPIO_Port GPIOB
#define IN2_Pin GPIO_PIN_1
#define IN2_GPIO_Port GPIOB
#define IN3_Pin GPIO_PIN_2
#define IN3_GPIO_Port GPIOB
#define IN4_Pin GPIO_PIN_10
#define IN4_GPIO_Port GPIOB
#define SC_CS_Pin GPIO_PIN_12
#define SC_CS_GPIO_Port GPIOB
#define WR_Pin GPIO_PIN_14
#define WR_GPIO_Port GPIOB
#define SC_PWR_Pin GPIO_PIN_6
#define SC_PWR_GPIO_Port GPIOC
#define SC_RST_Pin GPIO_PIN_7
#define SC_RST_GPIO_Port GPIOC
#define SW1_Pin GPIO_PIN_2
#define SW1_GPIO_Port GPIOD
#define SW1_EXTI_IRQn EXTI2_IRQn
#define BA_EN_Pin GPIO_PIN_3
#define BA_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
// 全局调试缓冲区
extern char debug_buffer[128];
extern uint8_t Flash_Buffer[1500];      //flash寄存器
extern uint32_t low_slope,low_intercept,mid_slope,mid_intercept,high_slope,high_intercept;   //拟合参数
extern float real_low_intercept,real_mid_intercept,real_high_intercept;                      //真实截距
extern uint32_t new_value;
extern uint8_t range_lock;                     //量程锁
extern uint8_t range_high;                     //高量程启用标志位
extern uint8_t range_mid;                      //中量程启用标志位
extern uint8_t range_low;                      //低量程启用标志位
// 函数声明
void Debug_Print(const char* format, ...);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
