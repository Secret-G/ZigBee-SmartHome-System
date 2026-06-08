#ifndef __DELAY_H
#define __DELAY_H

#include "main.h"

// 函数声明
void delay_init(void);          // 延时初始化（必须先调用）
void delay_ms(uint32_t ms);     // 毫秒级延时（支持FreeRTOS）
void delay_us(uint32_t us);     // 微秒级延时

#endif

