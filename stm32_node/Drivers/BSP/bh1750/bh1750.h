#ifndef __BH1750_H
#define __BH1750_H

#include "main.h"
#include "iic.h"
#include "usart.h"

// BH1750 7位地址（ADDR接GND=0x23，接VCC=0x46）
#define BH1750_ADDR    0x23  

// BH1750指令
#define BH1750_POWER_ON    0x01  // 上电
#define BH1750_H_RES_MODE  0x10  // 连续高分辨率模式

// 函数声明
void BH1750_Init(void);          // BH1750初始化（用模拟I2C）
float BH1750_ReadLux(void);      // 读取光照值（lux）
void BH1750_Send_Light_Frame(void);
#endif
