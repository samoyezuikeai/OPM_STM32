/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
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
#include "spi.h"

/* USER CODE BEGIN 0 */
#include "Range_Ctrl.h"

/* AD7791寄存器地址定义 */
#define AD7791_REG_COMM        0x00    // 通信寄存器（写操作）
#define AD7791_REG_STATUS      0x00    // 状态寄存器（读操作）
#define AD7791_REG_MODE        0x01    // 模式寄存器
#define AD7791_REG_FILTER      0x02    // 滤波器寄存器
#define AD7791_REG_DATA        0x03    // 数据寄存器

uint32_t Raw_Value;                         //ADC读取生值
float voltage;                              //AD7791读取电压
uint8_t buffer_filled = 0;                  //ADC缓存满了标志位
uint8_t buffer_index =0;                       //缓存索引
uint32_t range_switch_start_time = 0;       //	切换量程启动时间（消抖）
uint8_t OVER_RANGE = 0;                     //超出量程状态标志位
uint8_t UNDER_RANGE = 0;                    //向下超出量程状态标志位
uint32_t range_debounce_start_time = 0;

extern uint8_t UART_Stage;                    //串口发送状态标志位
extern uint32_t adc_buffer[10]; //ADC缓存
extern uint8_t range_lock;                    //量程锁
extern uint8_t range_high;                    //高量程启用标志位
extern uint8_t range_mid;                     //中量程启用标志位
extern uint8_t range_low;                     //低量程启用标志位
extern uint8_t change_flag;                   //量程改变标志位
extern uint8_t range;                              //量程

/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}
/* SPI2 init function */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PA4     ------> SPI1_NSS
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }
  else if(spiHandle->Instance==SPI2)
  {
  /* USER CODE BEGIN SPI2_MspInit 0 */

  /* USER CODE END SPI2_MspInit 0 */
    /* SPI2 clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB15     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI2_MspInit 1 */

  /* USER CODE END SPI2_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PA4     ------> SPI1_NSS
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);

  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
  else if(spiHandle->Instance==SPI2)
  {
  /* USER CODE BEGIN SPI2_MspDeInit 0 */

  /* USER CODE END SPI2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB15     ------> SPI2_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_15);

  /* USER CODE BEGIN SPI2_MspDeInit 1 */

  /* USER CODE END SPI2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/* AD7791函数*/
/**
  * @brief  SPI传输函数
  * @param  data: 要发送的数据
  * @retval 接收到的数据
  */
uint8_t AD7791_SPI_Transfer(uint8_t data)
{
    uint8_t rxData;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rxData, 1, HAL_MAX_DELAY);
    return rxData;
}

/**
  * @brief  写入AD7791寄存器
  * @param  regAddr: 寄存器地址
  * @param  data: 要写入的数据
  * @retval None
  */
void AD7791_WriteRegister(uint8_t regAddr, uint8_t data)
{
    uint8_t commReg;

    // 构建通信寄存器值
    // WEN=0, RS[1:0]=寄存器地址, R/W=0(写), CREAD=0, CH[1:0]=00(差分输入)
    commReg = (regAddr << 4) | 0x00;

    // 拉低CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(1);

    // 写入通信寄存器
    AD7791_SPI_Transfer(commReg);
    // 写入数据
    AD7791_SPI_Transfer(data);

    // 拉高CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(1);
}

/**
  * @brief  复位AD7791
  * @param  None
  * @retval None
  */
void AD7791_Reset(void)
{
    // 写入32个1来复位AD7791
    // 拉低CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    for(uint8_t i = 0; i < 4; i++)
    {
        AD7791_SPI_Transfer(0xFF);
    }

    // 拉高CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(1);
}

/**
  * @brief  AD7791初始化 - 单极性模式
  * @param  None
  * @retval None
  */
void AD7791_Init(void)
{
    // 1. 复位AD7791（通过写入32个1）
    AD7791_Reset();

    HAL_Delay(10);

    // 2. 配置滤波器寄存器
    // FS[2:0] = 100 (16.6Hz更新速率，同时抑制50Hz/60Hz)
    // CDIV[1:0] = 00 (正常模式)
    AD7791_WriteRegister(AD7791_REG_FILTER, 0x04);  // 采样频率选择

    // 3. 配置模式寄存器 - 改为单极性模式
    // MD[1:0] = 00 (连续转换模式)
    // BO = 0 (burnout电流禁用)
    // U/B = 1 (单极性模式) - 关键修改
    // BUF = 1 (缓冲模式)
    AD7791_WriteRegister(AD7791_REG_MODE, 0x06);    // 0b00000110 = 0x06

    // 4. 等待第一次转换完成
    HAL_Delay(50);
}

/**
  * @brief  读取AD7791寄存器
  * @param  regAddr: 寄存器地址
  * @retval 读取到的8位数据
  */
uint8_t AD7791_ReadRegister(uint8_t regAddr)
{
    uint8_t commReg;
    uint8_t data;

    // 构建通信寄存器值：读取指定寄存器
    // WEN=0, RS[1:0]=寄存器地址, R/W=1(读), CREAD=0, CH[1:0]=00
    commReg = (regAddr << 4) | 0x08;

    // 拉低CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(1);

    // 写入通信寄存器
    AD7791_SPI_Transfer(commReg);
    // 读取数据
    data = AD7791_SPI_Transfer(0xFF);

    // 拉高CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return data;
}

/**
  * @brief  读取AD7791数据
  * @param  None
  * @retval 24位ADC数据
  */
uint32_t AD7791_ReadData(void)
{
    uint8_t commReg;
    uint8_t data[3];
    uint32_t result = 0;

    // 等待数据就绪（通过DRDY引脚或状态寄存器）
    // 方法1：通过DRDY硬件引脚检测

    while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
    {
        // 等待DRDY变低，表示数据就绪
        HAL_Delay(1);
    }


    // 构建通信寄存器值：读取数据寄存器
    // WEN=0, RS[1:0]=11(数据寄存器), R/W=1(读), CREAD=0, CH[1:0]=00
    commReg = (AD7791_REG_DATA << 4) | 0x08;
    // 拉低CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(1);


    // 写入通信寄存器
    AD7791_SPI_Transfer(commReg);

    // 读取24位数据
    data[0] = AD7791_SPI_Transfer(0xFF);  // 高8位
    data[1] = AD7791_SPI_Transfer(0xFF);  // 中8位
    data[2] = AD7791_SPI_Transfer(0xFF);  // 低8位

    // 拉高CS
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    // 组合24位数据
    result = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];

    return result;

}

/**
  * @brief  滑动平均滤波器 - 每次读取一个值并返回当前平均值
  * @param  None
  * @retval 滑动平均值
  */
uint32_t AD7791_ReadData_MovingAverage(void)
{
    uint32_t sum = 0;

    // 读取新值
    uint32_t new_value = AD7791_ReadData();

    // 更新缓冲区
    adc_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % 16;

    if(!buffer_filled && buffer_index == 0)
    {
        buffer_filled = 1;  // 缓冲区已填满
    }

    // 计算平均值，能显著降低信号的毛刺
    uint8_t count = buffer_filled ? 9 : buffer_index;
    for(uint8_t i = 0; i < count; i++)
    {
        sum += adc_buffer[i];
    }
    UART_Stage = 1;
    return sum / (count+1);
}

/**
  * @brief  将ADC原始数据转换为电压值（单极性模式）
  * @param  rawData: 24位原始数据
  * @param  vref: 参考电压
  * @retval 计算得到的电压值
  */
float AD7791_ConvertToVoltage(uint32_t rawData, float vref)
{
    // 单极性模式下的转换公式：电压 = (原始数据 / 2^24) × 参考电压
    return ((float)rawData / 16777216.0f) * vref;  // 2^24 = 16777216
}
/**
  * @brief  检查AD7791状态
  * @param  None
  * @retval 状态寄存器值
  */
uint8_t AD7791_CheckStatus(void)
{
    return AD7791_ReadRegister(AD7791_REG_STATUS);
}
void Example_ReadAD7791(void)
{

    // 读取原始数据
    Raw_Value = AD7791_ReadData();

    // 转换为电压（假设参考电压为2V）
    voltage = AD7791_ConvertToVoltage(Raw_Value, 2.0f);

    // 使用电压值...
}



//量程判断函数
void Range_judge(uint32_t averaged_adc_value)
{
	if(range == 1 && averaged_adc_value > OVER_THRESHOLD_HIGH)
	{
        range_high = 0;
        range_mid = 1;
        range_low = 0;
        //memset(adc_buffer, 0, sizeof(adc_buffer));  //切换量程则清空adc寄存器
        buffer_index = (range_lock ==1)? buffer_index : 0 ;
	}
	else if(range == 2 && averaged_adc_value > OVER_THRESHOLD_HIGH)
	{
        range_high = 1;
        range_mid = 0;
        range_low = 0;
        //memset(adc_buffer, 0, sizeof(adc_buffer));  //切换量程则清空adc寄存器
        buffer_index = (range_lock ==1)? buffer_index : 0 ;
	}
	else if(range == 2 && averaged_adc_value < UNDER_THRESHOLD_LOW)
	{
        range_high = 0;
        range_mid = 0;
        range_low = 1;
        //memset(adc_buffer, 0, sizeof(adc_buffer));  //切换量程则清空adc寄存器
        buffer_index = (range_lock ==1)? buffer_index : 0 ;
	}
	else if(range == 3 && averaged_adc_value < UNDER_THRESHOLD_LOW)
	{
        range_high = 0;
        range_mid = 1;
        range_low = 0;
        //memset(adc_buffer, 0, sizeof(adc_buffer));  //切换量程则清空adc寄存器
        buffer_index = (range_lock ==1)? buffer_index : 0 ;
	}
	else{
		//UART_SendString("range error!\r\n");
	}
}

/* USER CODE END 1 */
