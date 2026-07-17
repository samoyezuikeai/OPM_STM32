/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "spi.h"
#include "usart.h"
#include "button_stages.h"
#include "Range_Ctrl.h"
#include "lcd.h"


// 命令缓冲区
#define CMD_BUFFER_SIZE 128
extern uint8_t cmd_buffer[CMD_BUFFER_SIZE];
extern char debug_buffer[128];

extern char info_str[100];
extern uint8_t BA_Stage;
extern uint8_t BA_Ctrl;
extern uint8_t range_high;
extern uint8_t range_mid;
extern uint8_t range_low;
extern uint8_t range;

extern uint32_t adc_buffer; //ADC读取缓存
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();  //开启端口时钟
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED_Pin|SC_PWR_Pin|SC_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IN1_Pin|IN2_Pin|IN3_Pin|IN4_Pin
                          |SC_CS_Pin|WR_Pin|BA_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN2_Pin IN3_Pin IN4_Pin
                           SC_CS_Pin WR_Pin BA_EN_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN2_Pin|IN3_Pin|IN4_Pin
                          |SC_CS_Pin|WR_Pin|BA_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SC_PWR_Pin SC_RST_Pin */
  GPIO_InitStruct.Pin = SC_PWR_Pin|SC_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SW1_Pin */
  GPIO_InitStruct.Pin = SW1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

}

/* USER CODE BEGIN 2 */





void BA_ctrl(void)
{
    if(BA_Stage == 1)
        HAL_GPIO_WritePin(BA_EN_GPIO_Port,BA_EN_Pin,GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(BA_EN_GPIO_Port,BA_EN_Pin,GPIO_PIN_RESET);
}

void LowPWR_sign(uint8_t i)
{
    while(i>0)
    {
        HAL_GPIO_WritePin(GPIOC,LED_Pin,GPIO_PIN_RESET);
        HAL_Delay(300);
        HAL_GPIO_WritePin(GPIOC,LED_Pin,GPIO_PIN_SET);
        HAL_Delay(300);
        i--;
    }
    HAL_GPIO_WritePin(GPIOC,LED_Pin,GPIO_PIN_SET);
}

void Check_Press(void)
{

}


void Handle_Long_Press(void)
{
    if(BA_Stage == 1)
    {
        // 启动状态下长按 -> 进入休眠（假关机）
        BA_Stage = 2;
        UART_SendString("Long Press - Entering Stop Mode\r\n");

        // 关闭屏幕
        LCD_RES_Clr();
        HAL_Delay(50);

        Enter_Stop_Mode();
    }
    else
    {
        UART_SendString("Error: Invalid BA_Stage in Handle_Long_Press\r\n");
    }
}

void Handle_Short_Press(void)
{
    UART_SendString("Short Press Detected\r\n");
    // 添加短按功能，例如切换LED
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

void Enter_Stop_Mode(void)
{
    UART_SendString("Preparing Stop Mode...\r\n");

    // 确保所有串口数据发送完成
    HAL_Delay(50);

    // 关闭所有外设（按依赖顺序）
    HAL_SPI_DeInit(&hspi1);
    HAL_SPI_DeInit(&hspi2);

    // 关闭LCD（如果有背光控制引脚，也关闭）
    HAL_GPIO_WritePin(GPIOC, LED_Pin, GPIO_PIN_RESET);  // 关闭LED

    // 关闭其他耗电外设
    HAL_UART_DeInit(&huart1);  // 关闭串口（可选，唤醒后需要重新初始化）

    // 确保SW1中断使能（用于唤醒）
//    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
//
//    // 进入停止模式前，设置标志
//    BA_Stage = 2;
//
//    UART_SendString("Entering STOP now...\r\n");
//    HAL_Delay(10);
//
//    // 进入停止模式 - 等待中断唤醒
//    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
//
//    // ========== 唤醒后从这里继续执行 ==========
//
//    // 重新配置系统时钟（停止模式会关闭主时钟）
//    SystemClock_Config();
//
//    // 重新初始化外设
//    MX_GPIO_Init();
//    MX_USART1_UART_Init();
//    MX_SPI1_Init();
//    MX_SPI2_Init();

    // 重新初始化LCD（关键！）
    //LCD_Init();                      // 初始化LCD硬件
    //LCD_StartPage_Init();            // 初始化显示内容
    //LCD_DisplayOn();                 // 打开显示


//    // 恢复状态
//    BA_Stage = 1;                    // 回到启动状态
//
//    UART_SendString("System Waked Up - LCD Restored\r\n");

    // 清除可能残留的按键中断
    button_state = BUTTON_IDLE;
    press_flag = 0;
    HAL_GPIO_WritePin(BA_EN_GPIO_Port, BA_EN_Pin, GPIO_PIN_RESET);
}

void Range_switch(void)
{
    if(range_lock ==0)
    {
    	if(range_high == 1) {
    		HAL_GPIO_WritePin(GPIOB,IN2_Pin,GPIO_PIN_SET);
    		HAL_GPIO_WritePin(GPIOB,IN1_Pin,GPIO_PIN_RESET);
    		HAL_GPIO_WritePin(GPIOB,IN3_Pin,GPIO_PIN_RESET);
    		HAL_GPIO_WritePin(GPIOB,IN4_Pin,GPIO_PIN_RESET);
    		range = 3;
    	} else if(range_mid == 1) {
    		HAL_GPIO_WritePin(GPIOB,IN3_Pin,GPIO_PIN_SET);
    		HAL_GPIO_WritePin(GPIOB,IN4_Pin,GPIO_PIN_SET);
    		HAL_GPIO_WritePin(GPIOB,IN1_Pin,GPIO_PIN_RESET);
    		HAL_GPIO_WritePin(GPIOB,IN2_Pin,GPIO_PIN_RESET);
    		range = 2;
    	} else if(range_low == 1) {
    		HAL_GPIO_WritePin(GPIOB,IN1_Pin,GPIO_PIN_SET);
    		HAL_GPIO_WritePin(GPIOB,IN4_Pin,GPIO_PIN_SET);
    		HAL_GPIO_WritePin(GPIOB,IN2_Pin,GPIO_PIN_RESET);
    		HAL_GPIO_WritePin(GPIOB,IN3_Pin,GPIO_PIN_RESET);
    		range = 1;
    	} else {
    		range = 0;
    	}

    }
}

uint8_t Power_On_Check(void)
{
    uint32_t press_start_time;
    uint32_t current_time;

    // 先检查SW1是否按下（低电平）
    if(HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET)

    // SW1按下，开始计时
    press_start_time = HAL_GetTick();

    // 等待长按时间（1500ms），期间持续检测
    while(1)
    {
        current_time = HAL_GetTick();

        // 检查是否提前释放（误触）
        if(HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_SET)
        {
            // 提前释放，认为是误触，不启动
            return 0;
        }

        // 检查是否达到长按时间
        if((current_time - press_start_time) >= 1000)  // 1秒长按
        {
            // 确认长按，拉高BA_EN自锁
            HAL_GPIO_WritePin(BA_EN_GPIO_Port, BA_EN_Pin, GPIO_PIN_SET);
            BA_Stage = 1;
            HAL_Delay(100);  // 等待稳定

//            // 等待按键释放（避免触发中断）
//            while(HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET);
//            HAL_Delay(50);  // 消抖

            return 1;  // 返回1表示正常启动
        }
    }
}


// 按键状态变量定义
volatile Button_State button_state = BUTTON_IDLE;
volatile uint32_t button_press_start_time = 0;
volatile uint8_t press_flag = 0;              // 0=无事件, 1=短按, 2=长按
volatile uint32_t last_interrupt_time = 0;

// 外部中断回调函数 - 双边沿触发
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_time = HAL_GetTick();

    if(GPIO_Pin != SW1_Pin)
        return;

    // 软件消抖
    if((current_time - last_interrupt_time) < BTN_DEBOUNCE_TIME)
        return;
    last_interrupt_time = current_time;

    // 读取当前引脚状态
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin);

    if(pin_state == GPIO_PIN_RESET)
    {
        // 下降沿：按键按下
        if(button_state == BUTTON_IDLE)
        {
            button_press_start_time = current_time;
            button_state = BUTTON_PRESSED;
            press_flag = 0;
        }
    }
    else
    {
        // 上升沿：按键释放
        if(button_state == BUTTON_PRESSED)
        {
            uint32_t press_duration = current_time - button_press_start_time;

            if(press_duration >= BTN_LONG_PRESS_TIME)
            {
                // 长按释放（但长按已在超时处理中触发，这里忽略）
            }
            else if(press_duration >= BTN_SHORT_PRESS_TIME)
            {
                // 短按（500ms ~ 1500ms）
                press_flag = 1;
            }
            else
            {
                // 超短按（<500ms），视为无效或短按
                press_flag = 1;
            }
            button_state = BUTTON_IDLE;
        }
        else if(button_state == BUTTON_WAIT_RELEASE)
        {
            // 长按后的释放
            button_state = BUTTON_IDLE;
            press_flag = 0;
        }
    }
}

// 主循环调用 - 处理长按超时（不轮询引脚）
void Button_Process(void)
{
    if(button_state == BUTTON_PRESSED)
    {
        uint32_t current_time = HAL_GetTick();
        uint32_t press_duration = current_time - button_press_start_time;

        // 长按超时检测（不需要轮询引脚，只检测时间）
        if(press_duration >= BTN_LONG_PRESS_TIME)

        {
            press_flag = 2;
            button_state = BUTTON_WAIT_RELEASE;  // 等待释放状态
        }
    }

    // 处理按键事件
    if(press_flag == 1)
    {
        press_flag = 0;           // 清除标志
        Handle_Short_Press();     // 执行短按处理
    }
    else if(press_flag == 2)
    {
        press_flag = 0;           // 清除标志
        Handle_Long_Press();      // 执行长按处理
    }
}
/* USER CODE END 2 */
