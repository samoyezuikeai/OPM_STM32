/*
 * lcd.h
 *
 *  Created on: Mar 2, 2026
 *      Author: LQHW
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define USE_HORIZONTAL 0  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#define LCD_W 240
#define LCD_H 240

#define LCD_RES_Clr()  HAL_GPIO_WritePin(GPIOC, SC_RST_Pin, GPIO_PIN_RESET)//RES
#define LCD_RES_Set()  HAL_GPIO_WritePin(GPIOC, SC_RST_Pin, GPIO_PIN_SET)

#define LCD_DC_Clr()   HAL_GPIO_WritePin(GPIOB, WR_Pin, GPIO_PIN_RESET)//WR
#define LCD_DC_Set()   HAL_GPIO_WritePin(GPIOB, WR_Pin, GPIO_PIN_SET)

#define LCD_CS_Clr()   HAL_GPIO_WritePin(GPIOB, SC_CS_Pin, GPIO_PIN_RESET)//CS
#define LCD_CS_Set()   HAL_GPIO_WritePin(GPIOB, SC_CS_Pin, GPIO_PIN_SET)

#define LCD_BLK_Clr()  HAL_GPIO_WritePin(GPIOC, SC_PWR_Pin, GPIO_PIN_RESET)//BLK
#define LCD_BLK_Set()  HAL_GPIO_WritePin(GPIOC, SC_PWR_Pin, GPIO_PIN_SET)


//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE           	 0x001F
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458 //灰蓝色
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

extern char Num_Show[16];      // 用于功率显示
extern char ADC_Show[16];      // 用于 ADC 显示

void LCD_Writ_Bus(uint8_t dat);//模拟SPI时序
void LCD_WR_DATA8(uint8_t dat);//写入一个字节
void LCD_WR_DATA(uint16_t dat);//写入两个字节
void LCD_WR_REG(uint8_t dat);//写入一个指令
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);//设置坐标函数
void LCD_Init(void);//LCD初始化

void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);//填充函数
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);//指定区域填充颜色
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);//在指定位置画一条线
void LCD_DrawRectangle_Fast(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void LCD_ShowChar_Fast(u16 x, u16 y, u8 ch, u16 fc, u16 bc, u8 sizey);
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey);//显示两位小数变量
void LCD_StartPage_Init(void);//初始页面生成
void LCD_Run(void);
void LCD_UpdateDisplayBuffers(double power_val, uint32_t adc_val);
#endif /* INC_LCD_H_ */
