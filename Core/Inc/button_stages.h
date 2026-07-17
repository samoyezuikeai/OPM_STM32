#ifndef BUTTON_STAGES_H
#define BUTTON_STAGES_H

#include "stdint.h"
#include "stm32f1xx_hal.h"

// 按键状态机
typedef enum {
    BUTTON_IDLE = 0,        // 空闲状态
    BUTTON_PRESSED,         // 按下（消抖中）
    BUTTON_WAIT_RELEASE     // 已触发长按，等待释放
} Button_State;



#define BTN_DEBOUNCE_TIME       20
#define BTN_SHORT_PRESS_TIME    500
#define BTN_LONG_PRESS_TIME     1500

// 外部变量声明
extern volatile Button_State button_state;
extern volatile uint32_t button_press_start_time;
extern volatile uint8_t press_flag;           // 1=短按触发, 2=长按触发
extern volatile uint32_t last_interrupt_time;

// 函数声明
void Button_Init(void);
void Button_Process(void);      // 主循环中调用
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif
