#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "main.h"

// 模拟I2C引脚定义（核心：PA6=SCL，PA7=SDA）
#define I2C_SOFT_SCL_PIN    GPIO_PIN_6
#define I2C_SOFT_SDA_PIN    GPIO_PIN_7
#define I2C_SOFT_PORT       GPIOB

// 引脚操作宏（简化代码）
#define I2C_SCL_H()   HAL_GPIO_WritePin(I2C_SOFT_PORT, I2C_SOFT_SCL_PIN, GPIO_PIN_SET)
#define I2C_SCL_L()   HAL_GPIO_WritePin(I2C_SOFT_PORT, I2C_SOFT_SCL_PIN, GPIO_PIN_RESET)
#define I2C_SDA_H()   HAL_GPIO_WritePin(I2C_SOFT_PORT, I2C_SOFT_SDA_PIN, GPIO_PIN_SET)
#define I2C_SDA_L()   HAL_GPIO_WritePin(I2C_SOFT_PORT, I2C_SOFT_SDA_PIN, GPIO_PIN_RESET)
#define I2C_SDA_READ() HAL_GPIO_ReadPin(I2C_SOFT_PORT, I2C_SOFT_SDA_PIN)

// 函数声明
void I2C_Soft_Init(void);                  // 模拟I2C初始化
void I2C_Soft_Start(void);                 // 起始信号
void I2C_Soft_Stop(void);                  // 停止信号
uint8_t I2C_Soft_SendByte(uint8_t byte);   // 发送1字节，返回ACK状态
uint8_t I2C_Soft_ReadByte(uint8_t ack);    // 读取1字节，ack=1发送ACK，0发送NACK

#endif

