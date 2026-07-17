/*
 * flash.h
 *
 *  Created on: Mar 2, 2026
 *      Author: LQHW
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include "main.h"

// 存储布局定义 - 24字节每参数
#define PARAM_LEN       24      // 每个参数24字节有效数据
#define ADDR_STRIDE     8       // 地址间隔8字节（原有方式）

// 计算实际地址索引（i * 8）
#define SERIAL_IDX_START    0   // 序列号：索引0-23，地址0-184
#define VERSION_IDX_START   24  // 版本号：索引24-47，地址192-376

// ========== 波长表（15行，列：wave + A~E）==========
// 行数
#define WAVE_ROW_COUNT      15
// 每行内各字段占用的“index数量”
#define COEF_BLOCK_IDX_LEN  16   // POLY_A~POLY_E 每个块：24个index（192字节地址空间）
#define WAVE_BLOCK_IDX_LEN  16   // DETEC_WAVE 每个块：10个index（原来168-177）
// 每行总共占用的index数量
#define WAVETABLE_ROW_STRIDE_IDX  96  // 16 + 5*16 = 96

// 波长表起始index（紧跟serial+version后面）
#define WAVETABLE_IDX_START  48

// ---- 按行取字段起始index（row:0~3）----
#define WAVETABLE_WAVE_IDX(row) \
    (WAVETABLE_IDX_START + (row) * WAVETABLE_ROW_STRIDE_IDX)

// 波长后面依次是 A~E
#define WAVETABLE_A_IDX(row) \
    (WAVETABLE_WAVE_IDX(row) + WAVE_BLOCK_IDX_LEN + 0 * COEF_BLOCK_IDX_LEN)

#define WAVETABLE_B_IDX(row) \
    (WAVETABLE_WAVE_IDX(row) + WAVE_BLOCK_IDX_LEN + 1 * COEF_BLOCK_IDX_LEN)

#define WAVETABLE_C_IDX(row) \
    (WAVETABLE_WAVE_IDX(row) + WAVE_BLOCK_IDX_LEN + 2 * COEF_BLOCK_IDX_LEN)

#define WAVETABLE_D_IDX(row) \
    (WAVETABLE_WAVE_IDX(row) + WAVE_BLOCK_IDX_LEN + 3 * COEF_BLOCK_IDX_LEN)

#define WAVETABLE_E_IDX(row) \
    (WAVETABLE_WAVE_IDX(row) + WAVE_BLOCK_IDX_LEN + 4 * COEF_BLOCK_IDX_LEN)

/*
#define POLY_A_IDX_START    48  // POLY_A：索引48-71，地址384-568
#define POLY_B_IDX_START    72  // POLY_B：索引72-95，地址576-760
#define POLY_C_IDX_START    96  // POLY_C：索引96-119，地址768-952
#define POLY_D_IDX_START    120 // POLY_D：索引120-143，地址960-1144
#define POLY_E_IDX_START    144 // POLY_E：索引144-167，地址1152-1336
#define DETEC_WAVE_START    168 // DETEC_WAVE_START：索引168-177
*/

#define TOTAL_IDX_LEN 1500

// 默认参数值（字符串形式）
#define DEFAULT_POLY_A  "2.142707011E-25"
#define DEFAULT_POLY_B  "-1.538492660E-18"
#define DEFAULT_POLY_C  "3.123563564E-12"
#define DEFAULT_POLY_D  "1.134808737E-04"
#define DEFAULT_POLY_E  "-5.410235865E-01"
#define DEFAULT_DETEC_WAVE  "785"
#define DEFAULT_SERIAL  "QOPM360402"
#define DEFAULT_VERSION "2.4.0-2.4.0"

void WriteFlash(uint8_t Page, uint8_t* Buf, uint16_t len);
void ReadFlash(uint8_t Page, uint8_t* Buf, uint16_t len);

void ReadParam(void);
void ChangeParam(void);

void Param_Init(void);          //参数初始化
void Update_LowSlope_FromFlash(void);


float StrToFloat(char* buf);
void ReadParam_New(void);
void ChangeParam_New(void);
void Param_Init_New(void);
void ClearBufferRange(uint16_t start, uint16_t len);
void CopyStringToIndex(uint16_t idx_start, const char* str);
uint8_t Flash_HasValidParams(void);
// 新增：初始化默认参数到FLASH
void Flash_InitDefaultParams(void);
#endif /* INC_FLASH_H_ */
