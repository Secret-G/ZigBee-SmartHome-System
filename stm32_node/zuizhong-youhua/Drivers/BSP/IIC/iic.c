#include "iic.h"
#include "delay.h"  // 需确保工程中有us级延时函数（下面附实现）

// 模拟I2C初始化（PA6/PA7设为开漏输出）
void I2C_Soft_Init(void)
{
    GPIO_InitTypeDef gpio_conf = {0};
    
    // 使能GPIOA时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置SCL/SDA为开漏输出（I2C必须开漏）
    gpio_conf.Pin = I2C_SOFT_SCL_PIN | I2C_SOFT_SDA_PIN;
    gpio_conf.Mode = GPIO_MODE_OUTPUT_OD;  // 开漏输出
    gpio_conf.Pull = GPIO_PULLUP;          // 上拉电阻
    gpio_conf.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_SOFT_PORT, &gpio_conf);
    
    // 初始状态：总线空闲（SCL/SDA都拉高）
    I2C_SCL_H();
    I2C_SDA_H();
}

// 模拟I2C起始信号（SDA先拉低，再拉低SCL）
void I2C_Soft_Start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    delay_us(5);  // 延时确保时序（必须us级）
    I2C_SDA_L();
    delay_us(5);
    I2C_SCL_L();
}

// 模拟I2C停止信号（SCL先拉高，再拉高SDA）
void I2C_Soft_Stop(void)
{
    I2C_SDA_L();
    I2C_SCL_H();
    delay_us(5);
    I2C_SDA_H();
    delay_us(5);
}

// 模拟I2C发送1字节，返回0=收到ACK，1=未收到ACK
uint8_t I2C_Soft_SendByte(uint8_t byte)
{
    uint8_t ack = 1;
    uint8_t i;
    
    // 逐位发送（高位在前）
    for(i=0; i<8; i++)
    {
        if(byte & 0x80) I2C_SDA_H();
        else I2C_SDA_L();
        byte <<= 1;
        
        delay_us(2);
        I2C_SCL_H();  // 拉高SCL，让从机读取数据
        delay_us(5);
        I2C_SCL_L();  // 拉低SCL，准备下一位
        delay_us(2);
    }
    
    // 等待从机ACK
    I2C_SDA_H();  // 释放SDA，让从机拉低
    delay_us(2);
    I2C_SCL_H();
    delay_us(5);
    if(I2C_SDA_READ() == GPIO_PIN_SET) ack = 1;  // 未收到ACK
    else ack = 0;                                // 收到ACK
    I2C_SCL_L();
    delay_us(2);
    
    return ack;
}

// 模拟I2C读取1字节，ack=1发送ACK，0发送NACK
uint8_t I2C_Soft_ReadByte(uint8_t ack)
{
    uint8_t byte = 0;
    uint8_t i;
    
    I2C_SDA_H();  // 释放SDA，由从机控制
    for(i=0; i<8; i++)
    {
        byte <<= 1;
        I2C_SCL_H();
        delay_us(5);
        if(I2C_SDA_READ() == GPIO_PIN_SET) byte |= 0x01;
        I2C_SCL_L();
        delay_us(2);
    }
    
    // 发送ACK/NACK
    if(ack == 1) I2C_SDA_L();  // 发送ACK
    else I2C_SDA_H();          // 发送NACK
    delay_us(2);
    I2C_SCL_H();
    delay_us(5);
    I2C_SCL_L();
    I2C_SDA_H();  // 释放SDA
    
    return byte;
}
