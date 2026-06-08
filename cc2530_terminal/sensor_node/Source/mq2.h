#ifndef __MQ2_H__
#define __MQ2_H__

#include "hal_types.h"

// 初始化IO
void MQ2_Init(void);

// 读取数字量（DO）
// 返回 0 = 未检测到气体
// 返回 1 = 检测到气体
uint8 MQ2_GetDO(void);

// 读取模拟量百分比（AO）
// 返回 0~100
uint8 MQ2_GetAO(void);

#endif
