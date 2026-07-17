/*
 * lcd.c
 *
 *  Created on: Mar 2, 2026
 *      Author: LQHW
 */

#include "spi.h"
#include "lcd.h"
#include "lcd_font.h"
#include <stdio.h>
#include <string.h>
//#include "usart.h"
//#include "adc.h"

extern uint8_t range;
extern uint32_t g_detec_wave;
extern float real_P3_out;
extern float real_P2_out;
extern uint32_t averaged_adc_value;

#define LINE_BUF_SIZE 64
#define HLINE_BUF_SIZE 128
float num_show;
char Num_Show[16] = {0};   // 功率显示
char ADC_Show[16] = {0};   // ADC 显示
uint32_t CurrentPowerLength = 0;
uint32_t PerviousPowerLength = 0;

// 显示用的副本 - 中断只读这个
volatile float g_display_power = 0;
volatile uint32_t g_display_adc = 0;
volatile uint8_t g_display_ready = 0;  // 数据就绪标志

#define WAVE_Y        120U
#define RANGE_Y       150U
#define ADC_Y         180U
#define RANGE_LOCK_Y  210U

static void LCD_Delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    // 72MHz 下，约 7200 个时钟周期 = 100μs，72000 = 1ms
    for(uint32_t i = 0; i < ms; i++)
    {
        for(count = 0; count < 7200; count++);  // 约 1ms
    }
}

/*
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
*/
void LCD_Writ_Bus(uint8_t dat)
{
    // 确保CS在数据传输前有足够时间
    //HAL_Delay(1);
    LCD_CS_Clr();

    // 增加超时检查
    uint32_t timeout = 1000;
    HAL_SPI_Transmit(&hspi2, &dat, 1, timeout);

    // 确保数据完成传输
    while(__HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_BSY)) {
        if(timeout-- == 0) break;
    }

    LCD_CS_Set();
    //HAL_Delay(1);  // CS置高后的延时
}
/******************************************************************************
      函数说明：LCD写入一个字数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(uint16_t dat)
{
	LCD_Writ_Bus(dat>>8);
	LCD_Writ_Bus(dat);

}

/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(uint8_t dat)

{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+80);
		LCD_WR_DATA(y2+80);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+80);
		LCD_WR_DATA(x2+80);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
}

//初始化函数
void LCD_Init(void)
{
	LCD_RES_Clr();//复位
	HAL_Delay(50);       // 复位保持 50ms
	LCD_RES_Set();
	HAL_Delay(50);       // 复位保持 50ms

    // 再次复位确保稳定
    LCD_RES_Clr();
	HAL_Delay(50);       // 复位保持 50ms
    LCD_RES_Set();
	HAL_Delay(50);       // 复位保持 50ms

	LCD_BLK_Set();//打开背光
	//UART_SendString(&huart1, "LCD INIT\r\n");
	//HAL_Delay(150);

	//************* Start Initial Sequence **********//
    LCD_WR_REG(0x01);  // 软复位命令（如果支持）
    LCD_Delay_ms(20);       // 复位保持 20ms

	LCD_WR_REG(0x11); //Sleep out
	//HAL_Delay(120);              //Delay 120ms
	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x36);
	if(USE_HORIZONTAL==0)LCD_Writ_Bus(0x00);
	else if(USE_HORIZONTAL==1)LCD_Writ_Bus(0xC0);
	else if(USE_HORIZONTAL==2)LCD_Writ_Bus(0x70);
	else LCD_Writ_Bus(0xA0);

	LCD_WR_REG(0x3A);
	LCD_Writ_Bus(0x05);

	LCD_WR_REG(0xB2);
	LCD_Writ_Bus(0x1F);
	LCD_Writ_Bus(0x1F);
	LCD_Writ_Bus(0x00);
	LCD_Writ_Bus(0x33);
	LCD_Writ_Bus(0x33);

	LCD_WR_REG(0xB7);
	LCD_Writ_Bus(0x35);

	LCD_WR_REG(0xBB);
	LCD_Writ_Bus(0x2B);   //2b

	LCD_WR_REG(0xC0);
	LCD_Writ_Bus(0x2C);

	LCD_WR_REG(0xC2);
	LCD_Writ_Bus(0x01);

	LCD_WR_REG(0xC3);
	LCD_Writ_Bus(0x0F);

	LCD_WR_REG(0xC4);
	LCD_Writ_Bus(0x20);   //VDV, 0x20:0v

	LCD_WR_REG(0xC6);
	LCD_Writ_Bus(0x13);   //0x13:60Hz

	LCD_WR_REG(0xD0);
	LCD_Writ_Bus(0xA4);
	LCD_Writ_Bus(0xA1);

	LCD_WR_REG(0xD6);
	LCD_Writ_Bus(0xA1);   //sleep in后，gate输出为GND

	LCD_WR_REG(0xE0);
	LCD_Writ_Bus(0xF0);
	LCD_Writ_Bus(0x04);
	LCD_Writ_Bus(0x07);
	LCD_Writ_Bus(0x04);
	LCD_Writ_Bus(0x04);
	LCD_Writ_Bus(0x04);
	LCD_Writ_Bus(0x25);
	LCD_Writ_Bus(0x33);
	LCD_Writ_Bus(0x3C);
	LCD_Writ_Bus(0x36);
	LCD_Writ_Bus(0x14);
	LCD_Writ_Bus(0x12);
	LCD_Writ_Bus(0x29);
	LCD_Writ_Bus(0x30);

	LCD_WR_REG(0xE1);
	LCD_Writ_Bus(0xF0);
	LCD_Writ_Bus(0x02);
	LCD_Writ_Bus(0x04);
	LCD_Writ_Bus(0x05);
	LCD_Writ_Bus(0x05);
	LCD_Writ_Bus(0x21);
	LCD_Writ_Bus(0x25);
	LCD_Writ_Bus(0x32);
	LCD_Writ_Bus(0x3B);
	LCD_Writ_Bus(0x38);
	LCD_Writ_Bus(0x12);
	LCD_Writ_Bus(0x14);
	LCD_Writ_Bus(0x27);
	LCD_Writ_Bus(0x31);

	LCD_WR_REG(0xE4);
	LCD_Writ_Bus(0x1D);   //使用240根gate  (N+1)*8
	LCD_Writ_Bus(0x00);   //设定gate起点位置
	LCD_Writ_Bus(0x00);   //当gate没有用完时，bit4(TMG)设为0

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x29);
}

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{
	   uint16_t width = xend - xsta;
	    uint16_t height = yend - ysta;
	    uint32_t total_pixels = width * height;

	    LCD_Address_Set(xsta, ysta, xend-1, yend-1);

	    // 分解颜色字节
	    uint8_t color_high = color >> 8;
	    uint8_t color_low = color & 0xFF;

	    // 一次性传输一行的数据
	    #define LINE_BUFFER_SIZE 240  // 假设最大宽度240

	    static uint8_t line_buffer[LINE_BUFFER_SIZE * 2];  // 每像素2字节

	    // 预填充一行颜色数据
	    for(uint16_t i = 0; i < width * 2; i += 2) {
	        line_buffer[i] = color_high;
	        line_buffer[i+1] = color_low;
	    }

	    LCD_CS_Clr();  // 开始数据传输

	    // 逐行传输
	    for(uint16_t row = 0; row < height; row++) {
	        HAL_SPI_Transmit(&hspi2, line_buffer, width * 2, 1000);
	    }

	    LCD_CS_Set();  // 结束数据传输
	}

// 快速字符显示函数 - 使用缓冲区批量传输
void LCD_ShowChar_Fast(u16 x, u16 y, u8 ch, u16 fc, u16 bc, u8 sizey)
{
    u8 temp, sizex, t, m = 0;
    u16 i, TypefaceNum;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    ch = ch - ' ';    // 得到偏移后的值

    // 设置显示区域
    LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1);

    // 创建缓冲区（存储一整行像素）

    static u8 line_buffer[LINE_BUF_SIZE];
    u16 buf_pos = 0;
    u16 current_row = 0;

    // 预计算颜色字节
    u8 fc_high = fc >> 8;
    u8 fc_low = fc & 0xFF;
    u8 bc_high = bc >> 8;
    u8 bc_low = bc & 0xFF;

    // 初始化：填充第一行背景色
    for(buf_pos = 0; buf_pos < sizex * 2; buf_pos += 2) {
        line_buffer[buf_pos] = bc_high;
        line_buffer[buf_pos + 1] = bc_low;
    }
    buf_pos = 0;

    // 按照原函数的顺序处理字模数据
    for(i = 0; i < TypefaceNum; i++) {
        // 获取字模数据
        switch(sizey) {
            case 12: temp = ascii_1206[ch][i]; break;
            case 16: temp = ascii_1608[ch][i]; break;
            case 24: temp = ascii_2412[ch][i]; break;
            case 32: temp = ascii_3216[ch][i]; break;
            default: return;
        }

        // 处理当前字节的8个位
        for(t = 0; t < 8; t++) {
            // 计算当前像素在行中的位置
            u16 pixel_col = buf_pos / 2;

            if(pixel_col < sizex) {
                if(temp & (0x01 << t)) {
                    // 前景色
                    line_buffer[buf_pos++] = fc_high;
                    line_buffer[buf_pos++] = fc_low;
                } else {
                    // 背景色
                    line_buffer[buf_pos++] = bc_high;
                    line_buffer[buf_pos++] = bc_low;
                }
            }

            m++;

            // 如果完成一行
            if(m == sizex) {
                // 发送这一行数据
                LCD_CS_Clr();
                HAL_SPI_Transmit(&hspi2, line_buffer, sizex * 2, 1000);
                LCD_CS_Set();

                // 重置缓冲区和计数器
                buf_pos = 0;
                m = 0;
                current_row++;

                // 为下一行填充背景色
                if(current_row < sizey) {
                    for(u16 j = 0; j < sizex * 2; j += 2) {
                        line_buffer[j] = bc_high;
                        line_buffer[j + 1] = bc_low;
                    }
                }
                break;
            }
        }
    }
}
/******************************************************************************
      函数说明：显示两位小数变量（优化版）
      入口数据：x,y显示坐标
                num 要显示小数变量
                len 要显示的位数（包括整数和小数部分，如25.67则len=4）
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      返回值：  无
******************************************************************************/
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{
    u8 sizex = sizey / 2;

    // 将浮点数乘以100并转换为整数（四舍五入）
    u32 num1;
    if (num >= 0) {
        num1 = (u32)(num * 100.0f + 0.5f);
    } else {
        num1 = (u32)(num * 100.0f - 0.5f);
    }

    // 分离整数和小数部分
    u32 integer_part = num1 / 100;
    u32 decimal_part = num1 % 100;

    // 计算整数部分的位数
    u8 int_digit_count = 0;
    u32 temp_int = integer_part;

    if (temp_int == 0) {
        int_digit_count = 1;
    } else {
        while (temp_int > 0) {
            int_digit_count++;
            temp_int /= 10;
        }
    }

    // 计算需要显示的小数点位置
    // len是总位数（包括整数和小数），小数固定2位
    u8 expected_int_digits = len - 2;  // 期望的整数部分位数

    // 当前位置
    u16 current_x = x;

    // 处理整数部分
    if (int_digit_count >= expected_int_digits) {
        // 整数部分位数足够或超过，显示实际整数
        u8 int_digits[10];
        u8 int_count = 0;

        temp_int = integer_part;
        if (temp_int == 0) {
            int_digits[int_count++] = 0;
        } else {
            while (temp_int > 0) {
                int_digits[int_count++] = temp_int % 10;
                temp_int /= 10;
            }
        }

        // 从高位到低位显示
        for (int8_t i = int_count - 1; i >= 0; i--) {
            LCD_ShowChar_Fast(current_x, y, int_digits[i] + '0', fc, bc, sizey);
            current_x += sizex;
        }
    } else {
        // 整数部分位数不足，需要显示前导空格
        u8 leading_spaces = expected_int_digits - int_digit_count;

        // 显示前导空格
        for (u8 i = 0; i < leading_spaces; i++) {
            LCD_ShowChar_Fast(current_x, y, ' ', fc, bc, sizey);
            current_x += sizex;
        }

        // 显示整数部分
        u8 int_digits[10];
        u8 int_count = 0;

        temp_int = integer_part;
        if (temp_int == 0) {
            int_digits[int_count++] = 0;
        } else {
            while (temp_int > 0) {
                int_digits[int_count++] = temp_int % 10;
                temp_int /= 10;
            }
        }

        // 从高位到低位显示
        for (int8_t i = int_count - 1; i >= 0; i--) {
            LCD_ShowChar_Fast(current_x, y, int_digits[i] + '0', fc, bc, sizey);
            current_x += sizex;
        }
    }

    // 显示小数点
    LCD_ShowChar_Fast(current_x, y, '.', fc, bc, sizey);
    current_x += sizex;

    // 显示两位小数（固定2位）
    u8 dec_tens = decimal_part / 10;
    u8 dec_ones = decimal_part % 10;

    LCD_ShowChar_Fast(current_x, y, dec_tens + '0', fc, bc, sizey);
    current_x += sizex;
    LCD_ShowChar_Fast(current_x, y, dec_ones + '0', fc, bc, sizey);
}

// 优化字符串显示函数
void LCD_ShowString_Fast(u16 x, u16 y, const u8 *str, u16 fc, u16 bc, u8 sizey)
{
    u8 sizex = sizey / 2;
    u16 start_x = x;

    while(*str != '\0') {
        // 检查是否需要换行
        if(x + sizex > LCD_W) {
            x = start_x;
            y += sizey;

            // 如果超出屏幕底部，停止显示
            if(y + sizey > LCD_H) break;
        }

        LCD_ShowChar_Fast(x, y, *str, fc, bc, sizey);
        x += sizex;
        str++;
    }
}

// 内部使用的垂直线绘制函数（已经设置好地址）
static void LCD_DrawFastVLine_Internal(u16 x, u16 y, u16 len, u8 color_high, u8 color_low)
{
    LCD_CS_Clr();

    // 垂直线可以直接循环发送，因为每行只有一个像素
    for(uint16_t i = 0; i < len; i++) {
        uint8_t buffer[2] = {color_high, color_low};
        HAL_SPI_Transmit(&hspi2, buffer, 2, 1000);
    }

    LCD_CS_Set();
}

// 内部使用的水平线绘制函数（已经设置好地址）
static void LCD_DrawFastHLine_Internal(u16 x, u16 y, u16 len, u8 color_high, u8 color_low)
{

    static uint8_t hline_buffer[HLINE_BUF_SIZE];

    // 预填充缓冲区
    for(uint16_t i = 0; i < HLINE_BUF_SIZE; i += 2) {
        hline_buffer[i] = color_high;
        hline_buffer[i + 1] = color_low;
    }

    uint32_t total_bytes = len * 2;
    uint32_t bytes_sent = 0;

    LCD_CS_Clr();

    while(bytes_sent < total_bytes) {
        uint16_t to_send = (total_bytes - bytes_sent > HLINE_BUF_SIZE) ?
                          HLINE_BUF_SIZE : (uint16_t)(total_bytes - bytes_sent);

        HAL_SPI_Transmit(&hspi2, hline_buffer, to_send, 1000);
        bytes_sent += to_send;
    }

    LCD_CS_Set();
}

//快速画方框函数
void LCD_DrawRectangle_Fast(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    // 确保坐标正确
    if(x1 > x2) { u16 temp = x1; x1 = x2; x2 = temp; }
    if(y1 > y2) { u16 temp = y1; y1 = y2; y2 = temp; }

    u16 width = x2 - x1 + 1;   // 矩形宽度（包含两端）
    u16 height = y2 - y1 + 1;  // 矩形高度（包含两端）

    // 预计算颜色字节
    u8 color_high = color >> 8;
    u8 color_low = color & 0xFF;

    // 绘制上边（水平线）
    if(width > 0) {
        LCD_Address_Set(x1, y1, x2, y1);
        LCD_DrawFastHLine_Internal(x1, y1, width, color_high, color_low);
    }

    // 绘制下边（水平线）
    if(width > 0 && height > 1) {
        LCD_Address_Set(x1, y2, x2, y2);
        LCD_DrawFastHLine_Internal(x1, y2, width, color_high, color_low);
    }

    // 绘制左边（垂直线） - 排除上下角的点（已经画过）
    if(height > 2) {
        LCD_Address_Set(x1, y1 + 1, x1, y2 - 1);
        LCD_DrawFastVLine_Internal(x1, y1 + 1, height - 2, color_high, color_low);
    }

    // 绘制右边（垂直线） - 排除上下角的点（已经画过）
    if(width > 0 && height > 2) {
        LCD_Address_Set(x2, y1 + 1, x2, y2 - 1);
        LCD_DrawFastVLine_Internal(x2, y1 + 1, height - 2, color_high, color_low);
    }
}

// 屏幕初始显示函数
void LCD_StartPage_Init(void)
{
    //uint8_t retry_count = 1;
        LCD_Init();
        //HAL_Delay(500);

        LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);  // ***整块屏幕的初始化决定背景色
        //HAL_Delay(100);

        // 显示测试文本
        LCD_ShowString_Fast(10, 10, (u8*)"OPM", WHITE, BLACK, 32);
        //HAL_Delay(200);

        LCD_DrawRectangle_Fast(10, 50, 130 , 110 ,BLACK);
        LCD_DrawRectangle_Fast(130, 80, 170 , 110 ,BLACK);

//        LCD_Address_Set(10, 150, 230, 130);
//        LCD_DrawFastHLine_Internal(10,150,220,0x00,0x00);
//        LCD_Address_Set(10, 190, 230, 170);
//        LCD_DrawFastHLine_Internal(10,190,220,0x00,0x00);
//
//        LCD_Fill(15, 55, 125, 105, BLACK);
//        LCD_Fill(202, 82, 238, 108, BLACK);
//        LCD_Fill(15, 115, 230, 145, BLACK);
//        LCD_Fill(15, 155, 230, 185, BLACK);
//        LCD_Fill(15, 195, 230, 225, BLACK);

        //LCD_ShowString_Fast(20, 65, (u8*)"Temp:", RED, BLACK, 32);
        LCD_ShowString_Fast(205, 75, (u8*)"m W", WHITE, BLACK, 16);
        LCD_ShowString_Fast(20, WAVE_Y, (u8*)"Wave:", WHITE, BLACK, 16);
        LCD_ShowString_Fast(20, RANGE_Y, (u8*)"Range:", WHITE, BLACK, 16);
        LCD_ShowString_Fast(20, ADC_Y, (u8*)"AD:", WHITE, BLACK, 16);
        LCD_ShowString_Fast(20, RANGE_LOCK_Y, (u8*)"Range_Lock:", WHITE, BLACK, 16);

        LCD_ShowString_Fast(80, RANGE_Y, (u8*)"Mid", WHITE, BLACK, 16);
        LCD_ShowString_Fast(120, RANGE_LOCK_Y, (u8*)"Lock", WHITE, BLACK, 16);
        //LCD_ShowString_Fast(80, 200, (u8*)"", RED, BLACK, 16);
}

void LCD_Run(void)
{
	    static uint32_t last_display_wave = UINT32_MAX;
	    char wave_buf[24];

	   // 读取本地副本
	    __disable_irq();
	    float local_power = g_display_power;
	    uint32_t local_adc = g_display_adc;
	    __enable_irq();

	    //正负判断，防止输出负数
	    local_power = (local_power>0)?local_power:0;

	if (g_detec_wave != last_display_wave)
	{
		snprintf(wave_buf, sizeof(wave_buf), "Wave: %lu nm",
		         (unsigned long)g_detec_wave);
		LCD_Fill(20, WAVE_Y, LCD_W, WAVE_Y + 16U, BLACK);
		LCD_ShowString_Fast(20, WAVE_Y, (u8*)wave_buf, WHITE, BLACK, 16);
		last_display_wave = g_detec_wave;
	}

	if(range_lock ==0)
		LCD_ShowString_Fast(120, RANGE_LOCK_Y, (u8*)"Free", WHITE, BLACK, 16);
	else
		LCD_ShowString_Fast(120, RANGE_LOCK_Y, (u8*)"Lock", WHITE, BLACK, 16);

    // 显示 ADC 值 (20, 160) 位置 - 使用已格式化的字符串
    LCD_ShowString_Fast(60, ADC_Y, (u8*)ADC_Show, WHITE, BLACK, 16);  // 显示 ADC 值
    //如果平均值超量程，则显示警告

    // ===== 超量程状态管理（防闪烁）=====
        static uint8_t last_overrange_status = 0xFF;  // 0xFF=初始化, 0=正常, 1=超量程
        uint8_t current_overrange = (averaged_adc_value == 0xFFFFFF) ? 1 : 0;

        // 只在状态变化时才刷新屏幕
        if (current_overrange != last_overrange_status) {
            if (current_overrange) {
                // 进入超量程状态：显示警告
                LCD_ShowString_Fast(120, ADC_Y, (u8*)"OverRange!", YELLOW, BLACK, 16);
            } else {
                // 退出超量程状态：清除警告区域（刷新为黑色背景）
                LCD_Fill(120, ADC_Y, 220, ADC_Y + 16U, BLACK);  // 根据字体高度调整y2  正常显示会在字符串之外不覆盖 要和初始背景色相同
            }
            last_overrange_status = current_overrange;
        }
    char temp_buf[16];

	//根据量程判断拟合方程和单位
	if(range==3)//高量程
	{
		//num_show = (float)(new_value/high_slope) + real_high_intercept;        //显示值
		snprintf(temp_buf,sizeof(temp_buf), "%.2f", (double)local_power);                       // 保留三位小数
		CurrentPowerLength = strlen(Num_Show);
		if(PerviousPowerLength - CurrentPowerLength > 0) LCD_Fill(15, 55, 175, 105, BLACK);
		LCD_ShowString_Fast(20, 65, (u8*)Num_Show, WHITE, BLACK, 32);    //数值显示
        LCD_ShowString_Fast(80, RANGE_Y, (u8*)"Hig", WHITE, BLACK, 16);
        LCD_ShowString_Fast(205, 75, (u8*)"m W", WHITE, BLACK, 16);
        //LCD_Fill(15, 55, 175, 105, BLACK);
        //LCD_ShowFloatNum1(20,65,real_P,4,RED, BLACK, 32);
        PerviousPowerLength = strlen(Num_Show);
	}
	else if(range==2)//中量程
	{
//		num_show = (float)(new_value/mid_slope) + real_mid_intercept;        //显示值
//		snprintf(Num_Show,10, "%.2f", num_show);                      // 保留三位小数

		snprintf(temp_buf,sizeof(temp_buf), "%.2f", (double)local_power);                       // 保留三位小数
		CurrentPowerLength = strlen(Num_Show);
		if(PerviousPowerLength - CurrentPowerLength > 0) LCD_Fill(15, 55, 175, 105, BLACK);
	    LCD_ShowString_Fast(20, 65, (u8*)Num_Show, WHITE, BLACK, 32);    //数值显示
        LCD_ShowString_Fast(80, RANGE_Y, (u8*)"Mid", WHITE, BLACK, 16);
        LCD_ShowString_Fast(205, 75, (u8*)"m W", WHITE, BLACK, 16);
        //LCD_ShowFloatNum1(20,65,real_P,4,RED, BLACK, 32);
        PerviousPowerLength = strlen(Num_Show);
	}
	else if(range==1)//低量程
	{
//		num_show = (float)(new_value/low_slope) + real_low_intercept;        //显示值
//		snprintf(Num_Show,sizeof(Num_Show), "%.2f", num_show);                      // 保留三位小数
		CurrentPowerLength = strlen(Num_Show);
		if(PerviousPowerLength - CurrentPowerLength > 0) LCD_Fill(15, 55, 175, 105, BLACK);
	    LCD_ShowString_Fast(20, 65, (u8*)Num_Show, WHITE, BLACK, 32);    //数值显示
        LCD_ShowString_Fast(80, RANGE_Y, (u8*)"Low", WHITE, BLACK, 16);
        LCD_ShowString_Fast(205, 75, (u8*)"u W", WHITE, BLACK, 16);
        PerviousPowerLength = strlen(Num_Show);
	}

}

// 在主循环中调用 - 安全地准备显示数据
void LCD_UpdateDisplayBuffers(double power_val, uint32_t adc_val)
{
    // 格式化到缓冲区
    snprintf(Num_Show, sizeof(Num_Show), "%.3f", (double)power_val);
    snprintf(ADC_Show, sizeof(ADC_Show), "%06lX", adc_val);  // 16进制显示，或改为 %lu 十进制

    // 原子更新显示变量
    __disable_irq();
    g_display_power = power_val;
    g_display_adc = adc_val;
    g_display_ready = 1;
    __enable_irq();
}
