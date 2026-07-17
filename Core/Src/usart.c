/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

static uint8_t tx_buffer[32];    //发送缓冲区
uint8_t Stage_Buffer[32];        //状态缓冲区
uint8_t Wavelenth_Buffer[32];    //波长缓冲区
uint8_t UART_TxBuffer[32];       //发送缓冲区（XY版）
uint8_t UART_RxBuffer[32];       //接收缓冲区

uint8_t rx_len=0;

// DMA接收全局变量
DMA_RX_Buffer dma_rx_buffer;
DMA_RX_Status dma_rx_status;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */
  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */
  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  USER_UART_Init();
  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */
  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */
  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */
  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */
  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/**
 * @brief 发送单个字节
 */
uint8_t UART_SendByte(uint8_t byte)
{
    if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_BUSY) {
        return 1;
    }
    return HAL_UART_Transmit(&huart1, &byte, 1, 100) != HAL_OK;
}

/**
 * @brief 发送字符串
 */
uint8_t UART_SendString(const char *str)
{
    if (str == NULL) return 1;

    uint16_t len = 0;
    while (str[len] != '\0') len++;

    if (len == 0) return 1;

    return HAL_UART_Transmit(&huart1, (uint8_t *)str, len, 1000) != HAL_OK ? 1 : 0;
}

/**
 * @brief 初始化DMA接收
 */
void UART_DMA_Rx_Init(void)
{
    // 清零缓冲区
    memset((void*)&dma_rx_buffer, 0, sizeof(dma_rx_buffer));
    dma_rx_status = DMA_RX_IDLE;

    // 启动DMA接收
    UART_DMA_Rx_Start();
}

/**
 * @brief 启动DMA接收
 */
void UART_DMA_Rx_Start(void)
{
    // 停止DMA（如果正在运行）
    HAL_UART_DMAStop(&huart1);

    // 重置缓冲区索引
    dma_rx_buffer.read_index = 0;
    dma_rx_buffer.write_index = 0;
    dma_rx_buffer.half_cplt_flag = 0;
    dma_rx_buffer.full_cplt_flag = 0;
    dma_rx_buffer.error_flag = 0;

    // 启动DMA接收（循环模式）
    if (HAL_UART_Receive_DMA(&huart1, dma_rx_buffer.buffer, UART_DMA_BUFFER_SIZE) == HAL_OK) {
        dma_rx_status = DMA_RX_RUNNING;
    } else {
        dma_rx_status = DMA_RX_ERROR;
    }
}

/**
 * @brief 停止DMA接收
 */
void UART_DMA_Rx_Stop(void)
{
    HAL_UART_DMAStop(&huart1);
    dma_rx_status = DMA_RX_IDLE;
}

/**
 * @brief 重置DMA接收
 */
void UART_DMA_Rx_Reset(void)
{
    UART_DMA_Rx_Stop();
    HAL_Delay(10);
    UART_DMA_Rx_Start();
}

/**
 * @brief 获取可读取的字节数
 * @return 可读取的字节数
 */
uint16_t UART_DMA_Available(void)
{
    // 使用DMA_CNDTR寄存器获取当前传输位置
    uint32_t dma_ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
    uint32_t write_index;

    // 计算当前写指针位置
    // CNDTR表示还有多少数据要传输，所以已传输的数据 = 总大小 - CNDTR
    write_index = UART_DMA_BUFFER_SIZE - dma_ndtr;

    // 确保在缓冲区范围内
    write_index = write_index % UART_DMA_BUFFER_SIZE;

    // 更新写指针
    dma_rx_buffer.write_index = write_index;

    uint32_t read_index = dma_rx_buffer.read_index;

    if (write_index >= read_index) {
        return (uint16_t)(write_index - read_index);
    } else {
        // 发生了回绕
        return (uint16_t)((UART_DMA_BUFFER_SIZE - read_index) + write_index);
    }
}

/**
 * @brief 读取一个字节（非阻塞）
 * @return 读取的字节，如果没有数据返回0
 */
uint8_t UART_DMA_ReadByte(void)
{
    uint8_t byte = 0;
    uint32_t read_index;

    if (UART_DMA_Available() == 0) {
        return 0;
    }

    // 原子操作
    read_index = dma_rx_buffer.read_index;
    byte = dma_rx_buffer.buffer[read_index];
    dma_rx_buffer.read_index = (read_index + 1) & UART_DMA_BUFFER_MASK;

    return byte;
}

/**
 * @brief 读取多个字节（非阻塞）
 * @param buffer 输出缓冲区
 * @param size 要读取的字节数
 * @return 实际读取的字节数
 */
uint16_t UART_DMA_ReadBytes(uint8_t *buffer, uint16_t size)
{
    uint16_t available, read_len;
    uint32_t read_index;

    available = UART_DMA_Available();
    if (available == 0 || buffer == NULL) {
        return 0;
    }

    // 限制读取长度
    read_len = (size > available) ? available : size;

    read_index = dma_rx_buffer.read_index;

    if ((read_index + read_len) <= UART_DMA_BUFFER_SIZE) {
        // 不需要回绕
        memcpy(buffer, &dma_rx_buffer.buffer[read_index], read_len);
    } else {
        // 需要回绕
        uint16_t first_part = UART_DMA_BUFFER_SIZE - read_index;
        memcpy(buffer, &dma_rx_buffer.buffer[read_index], first_part);
        memcpy(buffer + first_part, dma_rx_buffer.buffer, read_len - first_part);
    }

    // 更新读索引
    dma_rx_buffer.read_index = (read_index + read_len) & UART_DMA_BUFFER_MASK;

    return read_len;
}

/**
 * @brief 读取一行数据（直到换行符）
 * @param buffer 输出缓冲区
 * @param max_size 缓冲区最大容量
 * @return 读取的字节数（不包括换行符）
 */
uint16_t UART_DMA_ReadLine(uint8_t *buffer, uint16_t max_size)
{
    uint32_t read_index = dma_rx_buffer.read_index;
    uint32_t write_index = dma_rx_buffer.write_index;
    uint16_t bytes_read = 0;

    while (bytes_read < max_size - 1) {
        if (read_index == write_index) {
            break; // 没有更多数据
        }

        uint8_t data = dma_rx_buffer.buffer[read_index];
        buffer[bytes_read++] = data;
        read_index = (read_index + 1) & UART_DMA_BUFFER_MASK;

        if (data == '\n') {
            break; // 找到换行符
        }
    }

    buffer[bytes_read] = '\0'; // 字符串结束符
    dma_rx_buffer.read_index = read_index;

    return bytes_read;
}

/**
 * @brief 查找指定字节
 * @param target 要查找的字节
 * @return 找到的位置（相对于读指针），-1表示未找到
 */
int32_t UART_DMA_FindByte(uint8_t target)
{
    uint16_t available;
    uint32_t read_index, i;

    available = UART_DMA_Available();
    if (available == 0) {
        return -1;
    }

    read_index = dma_rx_buffer.read_index;

    for (i = 0; i < available; i++) {
        uint32_t index = (read_index + i) & UART_DMA_BUFFER_MASK;
        if (dma_rx_buffer.buffer[index] == target) {
            return (int32_t)i;
        }
    }

    return -1;
}

/**
 * @brief 清空接收缓冲区
 */
void UART_DMA_Flush(void)
{
    // 直接将读指针移动到写指针位置
    dma_rx_buffer.read_index = dma_rx_buffer.write_index;
}

/**
 * @brief 获取缓冲区使用率
 * @return 已使用字节数
 */
uint16_t UART_DMA_GetBufferUsage(void)
{
    uint32_t read_index = dma_rx_buffer.read_index;
    uint32_t write_index = dma_rx_buffer.write_index;

    if (write_index >= read_index) {
        return (uint16_t)(write_index - read_index);
    } else {
        return (uint16_t)(UART_DMA_BUFFER_SIZE - (read_index - write_index));
    }
}

/**
  * @brief  接收完整一行后回显（行缓冲模式）
  * @note   等待接收到换行符（\n）后，将整行数据发送回去
  */
void UART_Receive_And_Echo_By_Line(void)
{

    static uint8_t rx_buffer[256];
    uint16_t line_length;

    // 尝试读取一行
    line_length = UART_DMA_ReadLine(rx_buffer, sizeof(rx_buffer));

    if(line_length > 0)
    {
        // 调试信息：显示接收到的内容
        UART_SendString("Received: ");
        HAL_UART_Transmit(&huart1, rx_buffer, line_length, 100);
        UART_SendString("\r\n");

        // 如果需要，可以添加命令处理逻辑
        // 例如：if(strncmp((char*)rx_buffer, "help", 4) == 0) { ... }
    }
}


//new
void USER_UART_Init(void)
{
  /*使能串口空闲中断*/
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1,UART_RxBuffer,32);
}

void UART_IDLE_Callback(UART_HandleTypeDef *huart)
{
  uint32_t tmp1, tmp2;

  tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE);
  tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE);

  if((tmp1 != RESET) && (tmp2 != RESET))
  {
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    HAL_UART_AbortReceive_IT(huart);
    /*Clear the DMA Stream pending flags.*/
    __HAL_DMA_CLEAR_FLAG(huart->hdmarx,__HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx));

    /* Process Unlocked */
    __HAL_UNLOCK(huart->hdmarx);

    if(huart->Instance == USART1)
    {
      /* get rx data len */
      rx_len = 32 - huart->hdmarx->Instance->CNDTR;
      HAL_UART_Receive_DMA(huart,UART_RxBuffer,32);
    }
  }
}

/* USER CODE END 1 */
