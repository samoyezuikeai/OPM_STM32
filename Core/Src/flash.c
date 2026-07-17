/*
 * flash.c
 *
 *  Created on: Mar 2, 2026
 *      Author: LQHW
 */

#include "flash.h"
#include "usart.h"
//uint8_t Read_Buf[44];             //flash读取寄存器

extern uint32_t Param_1,Param_2,Param_3,Param_4,Param_5,Param_6;   //拟合参数
extern float POLY_A, POLY_B, POLY_C, POLY_D, POLY_E;
extern uint8_t detec_wave;                        //检测波长
extern uint32_t g_detec_wave;                     // 缓存波长数值
//extern uint32_t low_slope,low_intercept,mid_slope,mid_intercept,high_slope,high_intercept;   //拟合参数
//extern uint32_t low_intercept,mid_intercept,high_intercept;                                  //截距*1000
//extern float real_low_intercept,real_mid_intercept,real_high_intercept;                      //真实截距


static uint8_t g_active_row = 0;

/*FLASH写入程序 - 使用32位字编程*/
void WriteFlash(uint8_t Page, uint8_t* Buf, uint16_t len)
{
    HAL_FLASH_Unlock();

    // 擦除整页
    FLASH_EraseInitTypeDef FlashSet;
    FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
    FlashSet.Banks = FLASH_BANK_1;
    FlashSet.PageAddress = 0x0803F000 + 0x800 * Page;
    FlashSet.NbPages = 1;

    uint32_t PageError = 0;
    HAL_FLASHEx_Erase(&FlashSet, &PageError);

    // 32位字编程 - 每4字节打包成一个WORD
    HAL_StatusTypeDef status;
    for (int i = 0; i < len; i += 4)
    {
        // 小端序打包：低地址存低字节
        uint32_t data = 0;
        data |= (uint32_t)Buf[i];
        if (i + 1 < len) data |= (uint32_t)Buf[i + 1] << 8;
        if (i + 2 < len) data |= (uint32_t)Buf[i + 2] << 16;
        if (i + 3 < len) data |= (uint32_t)Buf[i + 3] << 24;

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   0x0803F000 + 0x800 * Page + i,  // 连续地址
                                   data);
        if(status != HAL_OK)
        {
            char err_buf[32];
            snprintf(err_buf, sizeof(err_buf), "Flash write err at %d\r\n", i);
            UART_SendString(err_buf);
            break;
        }
    }

    HAL_FLASH_Lock();
}

/*FLASH读取程序 - 地址连续*/
void ReadFlash(uint8_t Page, uint8_t* Buf, uint16_t len)
{
    for(int i = 0; i < len; i++)
    {
        Buf[i] = *(__IO uint8_t*)(0x0803F000 + 0x800 * Page + i);  // 字节地址连续读取
    }
}

//读取参数页参数
void ReadParam(void)
{
	memset(UART_TxBuffer, 0, sizeof(UART_TxBuffer));
	static int i = 0;                     //flash读写索引
	//读出所有的参量
	ReadFlash(Param_page,Flash_Buffer, 90);

	//定位，找到要读取的参量
	if(UART_RxBuffer[1] ==0x00)    //序列号
	{
		for(i = 0;i<10;i++)
		{
			 UART_TxBuffer[i] = Flash_Buffer[i];
		}
		HAL_UART_Transmit_DMA(&huart1,UART_TxBuffer,10);
	}
	else if(UART_RxBuffer[1] ==0x17) //版本号
	{
		for(i = 10;i<20;i++)
		{
			UART_TxBuffer[i-10] = Flash_Buffer[i];  //参数对应串口指令第二个字节
		}
		HAL_UART_Transmit_DMA(&huart1,UART_TxBuffer,10);
	}
	else                            //0x01到0x07七个参数
	{
		for(i = 0;i<10;i++)
		{
			UART_TxBuffer[i] = Flash_Buffer[i+10+UART_RxBuffer[1]*10];  //跳过版本号和序列号的20字节（不够长的话需要宽展）
		}
		HAL_UART_Transmit_DMA(&huart1,UART_TxBuffer,10);
	}

	//HAL_UART_Transmit_DMA(&huart1,UART_TxBuffer,16);
	//memset(UART_TxBuffer, 0, sizeof(UART_TxBuffer));
}

//修改参数页参数
void ChangeParam(void)
{
	static int i = 0;                     //flash读写索引

	//读出之前的参量
	ReadFlash(Param_page,Flash_Buffer, 90);

	//定位，写入要修改的参量
	if(UART_RxBuffer[1] ==0x00)    //写入序列号
	{
		for(i = 0;i<10;i++)
		{
			Flash_Buffer[i] = UART_RxBuffer[i+2];
		}
	}
	else if(UART_RxBuffer[1] ==0x17) //写入版本号
	{
		for(i = 10;i<20;i++)
		{
			Flash_Buffer[i] = UART_RxBuffer[i-8];  //参数对应串口指令第三个字节
		}
	}
	else                            //写入0x01到0x07七个参数
	{
		UART_SendString("set\r\n");
		for(i = 0;i<10;i++)
		{
			Flash_Buffer[i+10+UART_RxBuffer[1]*10] = UART_RxBuffer[i+2];  //跳过版本号和序列号的20字节（不够长的话需要宽展）
		}
	}

	//把修改好的参量重新写回去
	WriteFlash(Param_page, Flash_Buffer, 90);
	Param_Init();
}

void Param_Init(void)          //参数初始化
{
	uint8_t i;
	
	//读出之前的参量 low_slope,low_intercept,mid_slope,mid_intercept,high_slope,high_intercept
	ReadFlash(Param_page, Flash_Buffer, 90);
	
	//清零所有目标变量，防止多次调用累加
	Param_1 = 0;
	Param_2 = 0;
	Param_3 = 0;
	Param_4 = 0;
	Param_5 = 0;
	Param_6 = 0;
	
	//转换 Param_1: Flash_Buffer[30]~[39]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[30 + i];
		if (ch >= '0' && ch <= '9')
			Param_1 = Param_1 * 10 + (ch - '0');
		else
			break;
	}
	
	//转换 Param_2: Flash_Buffer[40]~[49]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[40 + i];
		if (ch >= '0' && ch <= '9')
			Param_2 = Param_2 * 10 + (ch - '0');
		else
			break;
	}
	
	//转换 Param_3: Flash_Buffer[50]~[59]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[50 + i];
		if (ch >= '0' && ch <= '9')
			Param_3 = Param_3 * 10 + (ch - '0');
		else
			break;
	}
	
	//转换 Param_4: Flash_Buffer[60]~[69]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[60 + i];
		if (ch >= '0' && ch <= '9')
			Param_4 = Param_4 * 10 + (ch - '0');
		else
			break;
	}
//	real_mid_intercept = (float)mid_intercept / 1000.0f;                //真实中量程截距
	
	//转换 Param_5: Flash_Buffer[70]~[79]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[70 + i];
		if (ch >= '0' && ch <= '9')
			Param_5 = Param_5 * 10 + (ch - '0');
		else
			break;
	}
	
	//转换 Param_6: Flash_Buffer[80]~[89]
	for (i = 0; i < 10; i++)
	{
		uint8_t ch = Flash_Buffer[80 + i];
		if (ch >= '0' && ch <= '9')
			Param_6 = Param_6 * 10 + (ch - '0');
		else
			break;
	}
//	real_high_intercept = (float)high_intercept / 1000.0f;              //真实高量程截距
}

/* 辅助函数：将字符串转换为float */
float StrToFloat(char* buf)
{
    return strtof(buf, NULL);
}

/* 读取参数页参数 - 修改支持24字节参数 */
void ReadParam_New(void)
{
	memset(UART_TxBuffer, 0, sizeof(UART_TxBuffer));
	static int i = 0;

	// 读出所有参数（168个索引）
	ReadFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);

	switch(UART_RxBuffer[1])
	{
		case 0x00:    // 序列号（24字节，索引0-23）
		{
			for(i = 0; i < PARAM_LEN; i++)
			{
				UART_TxBuffer[i] = Flash_Buffer[SERIAL_IDX_START + i];
			}
			HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, PARAM_LEN);
			break;
		}

		case 0x11:    // 版本号（24字节，索引24-47）
		{
			for(i = 0; i < PARAM_LEN; i++)
			{
				UART_TxBuffer[i] = Flash_Buffer[VERSION_IDX_START + i];
			}
			HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, PARAM_LEN);
			break;
		}

		case 0x01:    // 波长和A-E
		{
			g_active_row = UART_RxBuffer[2];
			switch(UART_RxBuffer[3])
			{
				case 0x01:  //Wave
				{
					for(i = 0; i < WAVE_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_WAVE_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, WAVE_BLOCK_IDX_LEN);
					break;
				}
				case 0x02:  //A
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_A_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, COEF_BLOCK_IDX_LEN);
					break;
				}
				case 0x03:  //B
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_B_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, COEF_BLOCK_IDX_LEN);
					break;
				}
				case 0x04:  //C
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_C_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, COEF_BLOCK_IDX_LEN);
					break;
				}
				case 0x05:  //D
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_D_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, COEF_BLOCK_IDX_LEN);
					break;
				}
				case 0x06:  //E
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_E_IDX(g_active_row) + i];
					}
					HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, COEF_BLOCK_IDX_LEN);
					break;
				}
			}
			break;
		}
		case 0x05:    // 当前Wave
		{
			g_active_row = UART_RxBuffer[2];
			for(i = 0; i < PARAM_LEN; i++)
			{
				UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_WAVE_IDX(g_active_row) + i];
			}
			HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, WAVE_BLOCK_IDX_LEN);
			break;
		}
		default:
		{
			break;
		}
	}
}


/* 修改参数页参数 - 修改支持24字节参数 */
void ChangeParam_New(void)
{
	static int i = 0;
    uint8_t param_valid = 0;  // 标志位：参数是否有效

	// 读出之前的所有参数（168个索引）
	ReadFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);

	switch(UART_RxBuffer[1])
	{
		case 0x00:    // 写入序列号（24字节，索引0-23）
		{
			for(i = 0; i < PARAM_LEN && (i + 2) < UART_RX_BUF_SIZE; i++)
			{
				Flash_Buffer[SERIAL_IDX_START + i] = UART_RxBuffer[i + 2];
			}
			param_valid = 1;
			break;
		}

		case 0x11:    // 写入版本号（24字节，索引24-47）
		{
			for(i = 0; i < PARAM_LEN && (i + 2) < UART_RX_BUF_SIZE; i++)
			{
				Flash_Buffer[VERSION_IDX_START + i] = UART_RxBuffer[i + 2];
			}
			param_valid = 1;
			break;
		}

		case 0x01:    // 写入波长和参数
		{
			g_active_row = UART_RxBuffer[2];
			switch(UART_RxBuffer[3])
			{
				case 0x01:  //Wave
				{
					for(i = 0; i < WAVE_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_WAVE_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}
					break;
				}
				case 0x02:  //A
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_A_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}
					break;
				}
				case 0x03:  //B
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_B_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}
					break;
				}
				case 0x04:  //C
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_C_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}
					break;
				}
				case 0x05:  //D
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_D_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}

					break;
				}
				case 0x06:  //E
				{
					for(i = 0; i < COEF_BLOCK_IDX_LEN; i++)
					{
						Flash_Buffer[WAVETABLE_E_IDX(g_active_row) + i] = UART_RxBuffer[i + 4];
					}
					break;
				}
			}
			param_valid = 1;
			break;
		}

		case 0x05:    // 设置当前波长
		{
			g_active_row = UART_RxBuffer[2];

			for(i = 0; i < WAVE_BLOCK_IDX_LEN; i++)
			{
				UART_TxBuffer[i] = Flash_Buffer[WAVETABLE_WAVE_IDX(g_active_row) + i];
			}

			HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, WAVE_BLOCK_IDX_LEN);
			Param_Init_New();
			break;
		}

		default:
		{
			UART_SendString("Unknown param\r\n");
		}
	}

	// 把修改好的参数重新写回去（168个索引）
    if (param_valid)
    {
        WriteFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);
        Param_Init_New();
        UART_SendString("Param saved\r\n");
    }
}

void Param_Init_New(void)
{
    // 修复：读取索引，确保覆盖到波长位置
    ReadFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);

    char temp_buf[17];

    // POLY_A
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_A_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%f", &POLY_A);

    // POLY_B
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_B_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%f", &POLY_B);

    // POLY_C
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_C_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%f", &POLY_C);

    // POLY_D
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_D_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%f", &POLY_D);

    // POLY_E
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_E_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%f", &POLY_E);

    // 波长
    memcpy(temp_buf, &Flash_Buffer[WAVETABLE_WAVE_IDX(g_active_row)], 16);
    temp_buf[16] = '\0';
    sscanf(temp_buf, "%u", &g_detec_wave);
    detec_wave = (uint8_t)g_detec_wave;
}

// 辅助函数：清空Flash_Buffer指定范围
void ClearBufferRange(uint16_t start, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        Flash_Buffer[start + i] = '\0';
    }
}

// 辅助函数：复制字符串到指定索引位置
void CopyStringToIndex(uint16_t idx_start, const char* str)
{
    uint16_t len = strlen(str);
    if (len > PARAM_LEN) len = PARAM_LEN;

    for (uint16_t i = 0; i < len; i++)
    {
        Flash_Buffer[idx_start + i] = str[i];
    }
    // 剩余空间已清零，确保字符串结束
}

uint8_t Flash_HasValidParams(void)
{
    // 读参数页到 Flash_Buffer
    ReadFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);
    // 如果序列号首字节为空/未写过，认为无效
    uint8_t first_byte = Flash_Buffer[SERIAL_IDX_START];
    return (first_byte != 0xFFU && first_byte != 0x00U);
}

/*
 * @brief 初始化默认参数到FLASH
 * @note  上电时调用，将默认参数写入FLASH
 *        仅在需要恢复出厂设置或首次烧录时调用
 */
void Flash_InitDefaultParams(void)
{
    UART_SendString("Init default params...\r\n");

    // Start from a clean buffer after reading an erased parameter area.
    memset(Flash_Buffer, 0, TOTAL_IDX_LEN);

    // 写入序列号（索引0-23）
    ClearBufferRange(SERIAL_IDX_START, PARAM_LEN);
    CopyStringToIndex(SERIAL_IDX_START, DEFAULT_SERIAL);

    // 写入版本号（索引24-47）
    ClearBufferRange(VERSION_IDX_START, PARAM_LEN);
    CopyStringToIndex(VERSION_IDX_START, DEFAULT_VERSION);


    // 写入默认检测波长
    ClearBufferRange(WAVETABLE_WAVE_IDX(0), WAVE_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_WAVE_IDX(0), DEFAULT_DETEC_WAVE);

    // 写入 POLY_A
    ClearBufferRange(WAVETABLE_A_IDX(0), COEF_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_A_IDX(0), DEFAULT_POLY_A);

    // 写入 POLY_B
    ClearBufferRange(WAVETABLE_B_IDX(0), COEF_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_B_IDX(0), DEFAULT_POLY_B);

    // 写入 POLY_C
    ClearBufferRange(WAVETABLE_C_IDX(0), COEF_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_C_IDX(0), DEFAULT_POLY_C);

    // 写入 POLY_D
    ClearBufferRange(WAVETABLE_D_IDX(0), COEF_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_D_IDX(0), DEFAULT_POLY_D);

    // 写入 POLY_E
    ClearBufferRange(WAVETABLE_E_IDX(0), COEF_BLOCK_IDX_LEN);
    CopyStringToIndex(WAVETABLE_E_IDX(0), DEFAULT_POLY_E);

    // 写入FLASH
    WriteFlash(Param_page, Flash_Buffer, TOTAL_IDX_LEN);

    UART_SendString("Default params saved to FLASH\r\n");
}

//KIMI CODE BEGIN//

/**
 * @brief 从Flash中读取0x02位置的ASCII参数，转换为数值存入low_slope
 * @note  存储位置：Flash_Buffer[26]~[35] (10字节)
 *        例如：存储"31323334" -> 转换为1234存入low_slope
 *        使用前需先调用ReadParam()或ReadFlash()确保Flash_Buffer数据已更新
 */
void Update_LowSlope_FromFlash(void)
{
    uint8_t i;
    uint32_t temp_value = 0;

    // 从Flash_Buffer[26]开始读取，最多10字节
    for (i = 0; i < 10; i++)
    {
        uint8_t ch = Flash_Buffer[30 + i];

        // 检查是否为有效数字字符 '0'~'9'
        if (ch >= '0' && ch <= '9')
        {
            temp_value = temp_value * 10 + (ch - '0');
        }
        else
        {
            // 遇到非数字字符，停止转换
            break;
        }
    }

    low_slope = temp_value;
}

//KIMI CODE END//



