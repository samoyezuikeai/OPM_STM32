#ifndef RANGE_CONTROL_H
#define RANGE_CONTROL_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif



/* 外部变量声明 */
//extern Range_State_t range_state;
extern uint32_t range_switch_start_time;
extern uint32_t range_debounce_start_time;
extern const uint32_t RANGE_SETTLE_TIME;
extern const uint32_t DEBOUNCE_TIME;

/* 滞后阈值定义 */
extern const uint32_t OVER_THRESHOLD_HIGH;
extern const uint32_t OVER_THRESHOLD_LOW;
extern const uint32_t UNDER_THRESHOLD_LOW;
extern const uint32_t UNDER_THRESHOLD_HIGH;

/* 函数声明 */
void Range_switch_Handler(void);
void Range_judge(uint32_t averaged_adc_value);
void Range_switch(void);
void range_show_H(void);
void Send_Sync_Measurement(void);

#ifdef __cplusplus
}
#endif

#endif /* RANGE_CONTROL_H */
