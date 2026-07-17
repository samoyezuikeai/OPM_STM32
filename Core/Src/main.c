/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "flash.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button_stages.h"
#include "Range_Ctrl.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// 2号PD校准数据表（ADC值 vs 功率值）
#define PD2_TABLE_SIZE  16

// ADC值表（必须按从小到大排序）
const uint32_t PD2_ADC_TABLE[PD2_TABLE_SIZE] = {
    275413,      // 0
    966302,      // 1
    1747266,     // 2
    3127446,     // 3
    3646239,     // 4
    5125308,     // 5
    6079006,     // 6
    7526277,     // 7
    8853456,     // 8  ← 0x8717D0 对应这里
    9716506,     // 9
    10832420,    // 10
    12027265,    // 11
    13078794,    // 12
    14295701,    // 13
    15214993,    // 14
    16007949     // 15
};

// 对应的功率值表（单位：μW 或 mW，根据实际）
const float PD2_POWER_TABLE[PD2_TABLE_SIZE] = {
    10.0f,       // 0
    33.3f,       // 1
    60.8f,       // 2
    103.0f,      // 3
    116.0f,      // 4
    161.0f,      // 5
    190.0f,      // 6
    234.0f,      // 7
    275.0f,      // 8  ← 0x8717D0 对应 275
    308.0f,      // 9
    355.0f,      // 10
    395.0f,      // 11
    441.0f,      // 12
    485.0f,      // 13
    519.0f,      // 14
    557.0f       // 15
};
extern uint8_t rx_len;


extern uint8_t BA_Stage;
//#define HAL_MAX_DELAY      0xFFFFFFFFU
uint8_t BA_Ctrl = 0;                       //上电控制
uint8_t BA_Stage = 0;                       //上电状态标志位

char info_str[20];  // 用于格式化输出，增加长度以适应ADC数据显示
char debug_buffer[128];
uint8_t UART_Stage = 0;                     //串口发送状态标志位
uint32_t new_value = 0;
uint32_t averaged_adc_value;               //平均AD值
uint32_t adc_buffer[10] = {0}; //ADC读取缓存
uint8_t Flash_Buffer[1500];      //flash寄存器
uint8_t Test_Buffer[16]= {1};   //测试寄存器
uint8_t range_lock = 0;                     //量程锁
uint8_t range_high = 1;                     //高量程启用标志位
uint8_t range_mid = 0;                      //中量程启用标志位
uint8_t range_low = 0;                      //低量程启用标志位
uint8_t range;                              //量程
uint8_t change_flag = 0;                    //量程改变标志位
uint8_t detec_wave;                        //检测波长
uint32_t g_detec_wave = 0;                  // 缓存波长数值

uint32_t Param_1,Param_2,Param_3,Param_4,Param_5,Param_6;   //拟合参数
float real_low_intercept,real_mid_intercept,real_high_intercept;                      //真实截距

//// 多项式拟合参数（对应 y = a*x^4 + b*x^3 + c*x^2 + d*x + e）
//#define POLY_A  (-6.41183384613951E-27)   // x^4 系数
//#define POLY_B  (+2.29588197534935E-19)    // x^3 系数
//#define POLY_C  (-2.21685207997818E-12)   // x^2 系数
//#define POLY_D  (+3.75709745416828E-05)     // x^1 系数
//#define POLY_E  (-2.26906490651210E-01)    // 常数项
float POLY_A, POLY_B, POLY_C, POLY_D, POLY_E;

float real_P;          //真实功率
float real_P_out = 0;  //真实功率输出（数据缓冲，防止内存混乱）


/* 去抖动参数 - 根据实际需求调整 */
const uint32_t RANGE_SETTLE_TIME = 200;     // 硬件稳定时间(ms)
const uint32_t HYSTERESIS_THRESHOLD = 0x20000; // 滞后阈值，避免在临界值抖动

/* 滞后阈值配置 */
const uint32_t OVER_THRESHOLD_HIGH = 0xFAE148;     // 向上切换阈值
const uint32_t OVER_THRESHOLD_LOW  = 0xF0E148;     // 向上切换滞后阈值
const uint32_t UNDER_THRESHOLD_LOW = 0x199999;     // 向下切换阈值
const uint32_t UNDER_THRESHOLD_HIGH = 0x239999;    // 向下切换滞后阈值

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void Debug_Print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(debug_buffer, format, args);
    va_end(args);
    UART_SendString(debug_buffer);
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
double Calculate_RealP_Horner(double averaged_adc_value);
float Calculate_Power_Linear(uint32_t adc_val);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // 初始化AD7791
  AD7791_Init();
  HAL_Delay(100);
  // 初始化按钮处理
  //Button_Init();
  Power_On_Check();    //上电按键检测

  // 初始化DMA接收
  UART_DMA_Rx_Init();

  // 参数区为空时只初始化一次，掉电重启后保留已经保存的参数
  if (!Flash_HasValidParams())
  {
    Flash_InitDefaultParams();
  }
  Param_Init_New();
  // ===== 调试：检查 POLY_A~E 是否正确加载 =====
  snprintf(debug_buffer, sizeof(debug_buffer),
           "POLY_A=%.6e\r\nPOLY_B=%.6e\r\nPOLY_C=%.6e\r\nPOLY_D=%.6e\r\nPOLY_E=%.6e\r\n",
           POLY_A, POLY_B, POLY_C, POLY_D, POLY_E);
  UART_SendString(debug_buffer);


  //ceshi
  for(int i = 0;i<=10;i++)
  {
	  Test_Buffer[i] = 5;
  }

  HAL_Delay(300);
  LCD_StartPage_Init();


  MX_TIM2_Init();
  range_lock = 0;
  Range_switch();

  memset(adc_buffer, 0, sizeof(adc_buffer));  //清空adc寄存器
  HAL_Delay(200);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
//	  uint16_t t0 = __HAL_TIM_GET_COUNTER(&htim2);   // 循环开始时间
//      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_1);
	  //ADC_Use();
	  //参数测试
	  //Update_LowSlope_FromFlash();

	  //memset(UART_TxBuffer, 0, 16);
	  //UART_SendString("before read\r\n");
	  //数据读取
	  new_value = 0;
	  new_value = AD7791_ReadData();
//	  snprintf(debug_buffer, sizeof(debug_buffer),
//	           "new_value=%d\r\n",new_value);
//	  UART_SendString(debug_buffer);
	  // 更新量程判断
	   //Range_judge(new_value);
	  //量程管理
	   Range_switch();


	    // ===== 使用局部静态变量，避免跨文件问题 =====
	    static uint32_t local_adc_buffer[10] = {0};
	    static uint8_t local_index = 0;
	    static uint8_t local_filled = 0;

	    // 更新缓冲区
	    local_adc_buffer[local_index] = new_value;
	    local_index++;

	    if (local_index >= 10) {
	        local_index = 0;
	        local_filled = 1;
	    }

	    // 计算平均值
	    uint8_t count = local_filled ? 10 : local_index;
	    uint64_t sum = 0;  // 使用64位确保不溢出

	    for(uint8_t i = 0; i < count; i++) {
	        sum += local_adc_buffer[i];
	    }

	    averaged_adc_value = (count > 0) ? (uint32_t)(sum / count) : new_value;

	    //计算实际功率值
	    // 将ADC值转换为float进行计算
	    double x = (double)averaged_adc_value;

	    // 计算多项式（使用优化版本）
	    real_P = Calculate_RealP_Horner(x);

	    // ===== 使用分段线性插值计算功率 =====
	    //real_P = Calculate_Power_Linear(averaged_adc_value);
	    real_P_out = (real_P > 0.0f) ? real_P : 0.0f;


//	    snprintf(debug_buffer, sizeof(debug_buffer), "%.2f\r\n", real_P_out);
//	    UART_SendString(debug_buffer);
	      //UART_SendString("after read\r\n");

	      // ===== 关键：安全更新显示缓冲区 =====
	      LCD_UpdateDisplayBuffers(real_P_out, averaged_adc_value);

//	      UART_TxBuffer[0] = (averaged_adc_value >> 24) & 0xFF;  // 最高字节（可能为0）
//	      UART_TxBuffer[1] = (averaged_adc_value >> 16) & 0xFF;  // 次高字节（如 0xFF）
//	      UART_TxBuffer[2] = (averaged_adc_value >> 8) & 0xFF;   // 第三字节（如 0x57）
//	      UART_TxBuffer[3] = averaged_adc_value & 0xFF;          // 最低字节（如 0x81）
//
//	      UART_TxBuffer[4] = range;
//	      UART_TxBuffer[5] = local_index;    // 修正：用 local_index 代替 buffer_index
//	      UART_TxBuffer[6] = range_lock;
//	      HAL_UART_Transmit_DMA(&huart1,UART_TxBuffer,8);

	    //LCD_Fill(15, 55, 175, 105, BLACK);
		LCD_Run();  // 传局部变量，安全

	  //UART_SendString("after Judge\r\n");

	  // 按键检测
	  //Check_Press();
	  Button_Process();
	  //UART_SendString("before DC\r\n");
	  //电源管理
	  //ADC_Use();
	  //Ba_EN_ctrl();
	  //BA_ctrl();
	  change_flag = 0;
	  //UART_SendString("after DC\r\n");



	  //UART_SendString("before print");

	  //UART_SendString("after print\r\n");

	    //HAL_Delay(50);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  command();         //指令处理
//	  uint16_t t1 = __HAL_TIM_GET_COUNTER(&htim2);   // 循环结束时间
//
//	      // 计算时间差（处理16位溢出）
//	      uint16_t elapsed;
//	      if (t1 >= t0) {
//	          elapsed = t1 - t0;           // 正常情况
//	      } else {
//	          elapsed = (65535 - t0) + t1 + 1;  // 溢出情况
//	      }
//
//	      // 每次循环结束发送时间（格式：T=1234μs）
//	      char time_buf[16];
//	      int len = snprintf(time_buf, sizeof(time_buf), "T=%u\r\n", elapsed);
//	      HAL_UART_Transmit(&huart1, (uint8_t*)time_buf, len, 10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void command(void)
{
	if(rx_len)
  {
	  switch(UART_RxBuffer[0])
	  {
	  	  case 0x03:
		  {
			  //HAL_UART_Transmit(&huart1, (uint8_t*)"Hello\r\n", 7, 1000);
					  switch(UART_RxBuffer[1])
					  {
						  case 0x01:                 //开始记录
						  {
//							  snprintf(debug_buffer, sizeof(debug_buffer), "%d，%.3f，%d，%.3f，%d，%.3f\r\n", low_slope,real_low_intercept,mid_slope,real_mid_intercept,high_slope,real_high_intercept);
//							  UART_SendString(debug_buffer);
						  }
						  break;

						  case 0x00:                 //停止记录
						  {

						  }
						  break;

						  case 0x11:				// 手动选择低量程
						  {
							  range_high = 0;
							  range_mid = 0;
							  range_low = 1;
							  Range_switch();
							  range = 1;
							  range_lock = 1;  //锁定量程


						  }
						  break;

						  case 0x12:					// 手动选择中量程
						  {
							  range_high = 0;
							  range_mid = 1;
							  range_low = 0;
							  Range_switch();
							  range = 2;
							  range_lock = 1;  //锁定量程
						  }
						  break;

						  case 0x13:					// 手动选择高量程
						  {
							  range_high = 1;
							  range_mid = 0;
							  range_low = 0;
							  Range_switch();
							  range = 3;
							  range_lock = 1;  //锁定量程
						  }
						  break;

						  case 0x14:					// 回到自动量程（开机默认自动量程）
						  {
							  range_lock = 0;  //解锁量程
							  Range_switch();
						  }
						  break;

						  default:
							  break;
					  }
				  }
				  break;

				  case 0x05:                           //读取flash的数据并发送
				  {
					  //HAL_UART_Transmit(&huart1, (uint8_t*)"Hello\r\n", 7, 1000);
					  switch(UART_RxBuffer[1])
					  {
					  	  case 0x13:                              //OPM运行状态
						  {
							    memset(UART_TxBuffer, 0, sizeof(UART_TxBuffer));

							    // 第0字节：供电状态
							    UART_TxBuffer[0] = BA_Stage;

							    // 第1字节：量程
							    UART_TxBuffer[1] = range;

							    // 第2-5字节：波长（4字节大端序）
							    // ✅ 修复：使用 g_detec_wave（uint32_t）而不是 detec_wave（uint8_t）
							    uint32_t wave_value = g_detec_wave;  // 使用 32 位变量
//							    UART_TxBuffer[2] = (wave_value >> 24) & 0xFF;
//							    UART_TxBuffer[3] = (wave_value >> 16) & 0xFF;
							    UART_TxBuffer[2] = (wave_value >> 8) & 0xFF;
							    UART_TxBuffer[3] = wave_value & 0xFF;

							    // 第6-8字节：ADC读数（3字节，AD7791是24位）
							    // new_value 是 uint32_t，取低24位
							    UART_TxBuffer[4] = (new_value >> 16) & 0xFF;  // 最高字节
							    UART_TxBuffer[5] = (new_value >> 8) & 0xFF;
							    UART_TxBuffer[6] = new_value & 0xFF;          // 最低字节

//							    // 第9-12字节：功率值（4字节float，IEEE 754）
//							    // 注意：STM32是小端序，直接memcpy即可
							    memcpy(&UART_TxBuffer[7], &real_P_out, sizeof(float));
//
//							    // 第13字节：量程锁定标志
//							    UART_TxBuffer[13] = range_lock;
//
//							    // 第14-15字节：保留
//							    UART_TxBuffer[14] = 0x00;
//							    UART_TxBuffer[15] = 0x00;

							    HAL_UART_Transmit_DMA(&huart1, UART_TxBuffer, 12);
							    break;
						  }
						  break;

						  default:
			  		  	  {
			  		  		ReadParam_New();   //读取参数
			  		  	  }
			  		  	  break;
					  }
				  }
				  break;

				  case 0x06:                           //将上位机发送数据的写入flash
				  {
					  HAL_UART_Transmit_DMA(&huart1,Test_Buffer,16);
					  ChangeParam_New();
				  }
				  break;

				  default:
					  break;
			  }
	  rx_len = 0;
  }
}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//	if(htim->Instance == TIM2)  // 判断是哪个定时器
//	{
//        // 原子复制：关中断保护
//        __disable_irq();
//        float local_copy = real_P_out;  // 复制到局部变量
//        __enable_irq();
//
//        LCD_Fill(15, 55, 175, 105, BLACK);
//        LCD_Run();  // 传局部变量，安全
//
//	}
//}

// 多项式计算函数：计算 y = a*x^4 + b*x^3 + c*x^2 + d*x + e
double Calculate_RealP_Horner(double averaged_adc_value)
{
    double x = averaged_adc_value;

    // Horner方法计算，减少运算次数，提高数值稳定性
    return POLY_E + x * (POLY_D + x * (POLY_C + x * (POLY_B + x * POLY_A)));
}



/**
 * @brief  分段线性插值计算功率
 * @param  adc_val: ADC平均值
 * @retval 计算得到的功率值
 */
float Calculate_Power_Linear(uint32_t adc_val)
{
    uint8_t i;
    float ratio;

    // 边界处理：小于最小值
    if(adc_val <= PD2_ADC_TABLE[0])
    {
        return PD2_POWER_TABLE[0];
    }

    // 边界处理：大于最大值
    if(adc_val >= PD2_ADC_TABLE[PD2_TABLE_SIZE - 1])
    {
        return PD2_POWER_TABLE[PD2_TABLE_SIZE - 1];
    }

    // 查找所在区间（线性查找，数据量小，足够快）
    for(i = 0; i < PD2_TABLE_SIZE - 1; i++)
    {
        if(adc_val < PD2_ADC_TABLE[i + 1])
        {
            break;  // 找到区间 [i, i+1]
        }
    }

    // 线性插值公式：
    // y = yᵢ + (yᵢ₊₁ - yᵢ) × (x - xᵢ) / (xᵢ₊₁ - xᵢ)
    ratio = (float)(adc_val - PD2_ADC_TABLE[i]) /
            (float)(PD2_ADC_TABLE[i + 1] - PD2_ADC_TABLE[i]);

    return PD2_POWER_TABLE[i] + ratio * (PD2_POWER_TABLE[i + 1] - PD2_POWER_TABLE[i]);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
