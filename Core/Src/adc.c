/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
#include "adc.h"

/* USER CODE BEGIN 0 */
extern uint8_t BA_Stage;         //上电状态标志位
extern uint8_t BA_Ctrl;          //上电控制
extern char info_str[100];       // 用于格式化输出，增加长度以适应ADC数据显示
uint16_t adc_value_ch1;          //adc通道一读数（type_c接入）
uint16_t adc_value_ch2;          //adc通道二读数（电池电压检测）
uint8_t type_c_con;              //type_c接入标志位
uint16_t adc_dma_buffer[2];      // DMA缓冲区用于存储两个通道的ADC值
volatile uint8_t adc_conversion_complete = 0;
float V_BA;                      //电池电压
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA1     ------> ADC1_IN1
    PA2     ------> ADC1_IN2
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_NORMAL;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA1     ------> ADC1_IN1
    PA2     ------> ADC1_IN2
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  初始化ADC DMA转换
  * @param  None
  * @retval None
  */
void ADC_DMA_Start(void)
{
    // 启动ADC DMA转换
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buffer, 2) != HAL_OK)
    {
        Error_Handler();
    }
}

void ADC_DMA_Stop(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    SOFT_UART_SendString("ADC DMA Stopped\r\n");
}

/**
  * @brief  ADC使用总函数 - DMA版本
  * @param  None
  * @retval None
  */
void ADC_Use(void)
{
    // 从DMA缓冲区读取两个通道的数据
    adc_value_ch1 = adc_dma_buffer[0];  // 通道1 (PA1)
    adc_value_ch2 = adc_dma_buffer[1];  // 通道2 (PA2)

     //发送ADC原始值到串口
    sprintf(info_str, "ADC_CH1: %4d (0x%04X), ADC_CH2: %4d (0x%04X)\r\n",
           adc_value_ch1, adc_value_ch1, adc_value_ch2, adc_value_ch2);
    UART_SendString(info_str);

    // 添加数据有效性检查
    if(adc_value_ch1 == 0 && adc_value_ch2 == 0)
    {
        // 如果都是0，可能是ADC没有启动或配置错误
        //UART_SendString("ADC ERROR: Both channels read 0, restarting ADC...\r\n");    //上电后读取的第一个值可能为0
        ADC_DMA_Start();
        return;
    }

    // 判断Type-C是否接入
    if(adc_value_ch1 >= 2500)
    {
        type_c_con = 1;
        //UART_SendString("Type-C: Connected\r\n");
    }
    else
    {
        type_c_con = 0;
        //UART_SendString("Type-C: Disconnected\r\n");
    }

    // 计算电池电压并发送
    V_BA = 6 * adc_value_ch2 / 4095.0;
    //sprintf(info_str, "Battery Voltage: %.2fV\r\n", V_BA);
    //UART_SendString(info_str);
}

/**
  * @brief  ADC转换完成回调函数
  * @param  hadc: ADC句柄
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    adc_conversion_complete = 1;
}

/**
  * @brief  根据是否接入type-c,电池电压控制开机关机
  * @param  None
  * @retval None
  */
void Ba_EN_ctrl(void)
{
    if(type_c_con == 1)
    {
        // 接入type_c则不连接电池
    	BA_Ctrl = 0;

    }
    else
    {
    	BA_Ctrl = 1;
    }
    // 添加串口输出语句，发送BA_Stage状态
    //sprintf(info_str, "BA_Stage: %d\r\n", BA_Stage);
    //UART_SendString(info_str);
}
/* USER CODE END 1 */
