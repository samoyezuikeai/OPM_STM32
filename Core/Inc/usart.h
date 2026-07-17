/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdarg.h>  // 用于可变参数函数
#include <stdio.h>   // 用于标准输入输出
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

/* USER CODE BEGIN Private defines */

#define UART_TX_BUFFER_SIZE    256
#define UART_RX_BUFFER_SIZE    256
// 接收缓冲区大小
#define UART_RX_BUFFER_SIZE     256

#define Param_page 25           //参数页

#ifndef UART_DMA_BUFFER_MASK
#define UART_DMA_BUFFER_MASK (UART_DMA_BUFFER_SIZE - 1)
#endif

#ifndef UART_DMA_BUFFER_SIZE
#define UART_DMA_BUFFER_SIZE 1024  // 根据需求调整大小
#endif

#ifndef CMD_BUFFER_SIZE
#define CMD_BUFFER_SIZE 128
#endif

// 检查是否是2的幂次方（编译时检查）
#if (UART_DMA_BUFFER_SIZE & (UART_DMA_BUFFER_SIZE - 1)) != 0
    #error "UART_DMA_BUFFER_SIZE must be power of 2"
#endif

#define UART_RX_BUF_SIZE 32

// DMA接收状态
typedef enum {
    DMA_RX_IDLE = 0,
    DMA_RX_RUNNING,
    DMA_RX_HALF_COMPLETE,
    DMA_RX_FULL_COMPLETE,
    DMA_RX_ERROR
}  DMA_RX_Status;

// DMA接收结构体
typedef struct {
    uint8_t buffer[UART_DMA_BUFFER_SIZE];  // 双缓冲区（前半+后半）
    volatile uint32_t write_index;         // DMA当前写入位置
    volatile uint32_t read_index;          // 用户读取位置
    volatile uint8_t half_cplt_flag;       // 半传输完成标志
    volatile uint8_t full_cplt_flag;       // 全传输完成标志
    volatile uint8_t error_flag;           // 错误标志
} DMA_RX_Buffer;

extern DMA_RX_Buffer dma_rx_buffer;
extern DMA_RX_Status dma_rx_status;

extern uint8_t Stage_Buffer[32];        //状态缓冲区
extern uint8_t Wavelenth_Buffer[32];    //波长缓冲区
extern uint8_t UART_TxBuffer[32];       //发送缓冲区（XY版）
extern uint8_t UART_RxBuffer[32];       //接收缓冲区

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
//=========发送函数==========
uint8_t UART_SendByte(uint8_t byte);
uint8_t UART_SendString(const char *str);
//========DMA接收函数========
void UART_DMA_Rx_Init(void);
void UART_DMA_Rx_Start(void);
void UART_DMA_Rx_Stop(void);
void UART_DMA_Rx_Reset(void);

uint16_t UART_DMA_Available(void);
uint8_t UART_DMA_ReadByte(void);
uint16_t UART_DMA_ReadBytes(uint8_t *buffer, uint16_t size);
int32_t UART_DMA_FindByte(uint8_t target);
uint16_t UART_DMA_ReadUntil(uint8_t* buffer, uint16_t max_size, uint8_t delimiter);
uint16_t UART_DMA_ReadLine(uint8_t *buffer, uint16_t max_size);
void UART_DMA_Flush(void);
// 回调函数（由CubeMX自动生成的中断调用）
void UART_DMA_RxHalfCompleteCallback(void);
void UART_DMA_RxCompleteCallback(void);
void UART_DMA_ErrorCallback(void);


/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

