#include "bh1750.h"
#include "stdio.h"
#include "delay.h"

// BH1750初始化（基于模拟I2C）
void BH1750_Init(void)
{
    // 先初始化模拟I2C
    I2C_Soft_Init();
    delay_ms(100);  // 等待传感器上电稳定
    
    // 发送上电指令
    I2C_Soft_Start();
    uint8_t ack = I2C_Soft_SendByte((BH1750_ADDR << 1) | 0);  // 写操作
    if(ack != 0)
    {
        printf("BH1750 上电指令发送失败！\r\n");
        I2C_Soft_Stop();
        return;
    }
    I2C_Soft_SendByte(BH1750_POWER_ON);
    I2C_Soft_Stop();
    delay_ms(10);
    
    // 发送高分辨率模式指令
    I2C_Soft_Start();
    ack = I2C_Soft_SendByte((BH1750_ADDR << 1) | 0);
    if(ack != 0)
    {
        printf("BH1750 模式设置失败！\r\n");
        I2C_Soft_Stop();
        return;
    }
    I2C_Soft_SendByte(BH1750_H_RES_MODE);
    I2C_Soft_Stop();
    delay_ms(120);  // 高分辨率模式需要120ms稳定
    
    printf("BH1750 初始化成功（模拟I2C PA6/PA7）\r\n");
}

// 读取光照值（基于模拟I2C）
float BH1750_ReadLux(void)
{
    uint8_t data[2] = {0};
    float lux = 0.0;
    
    // 发送读指令
    I2C_Soft_Start();
    uint8_t ack = I2C_Soft_SendByte((BH1750_ADDR << 1) | 1);  // 读操作
    if(ack != 0)
    {
       // printf("BH1750 读指令发送失败！\r\n");
        I2C_Soft_Stop();
        return 0.0;
    }
    
    // 读取2字节数据（第一个字节发ACK，第二个发NACK）
    data[0] = I2C_Soft_ReadByte(1);  // 高字节，发ACK
    data[1] = I2C_Soft_ReadByte(0);  // 低字节，发NACK
    I2C_Soft_Stop();
    
    // 计算光照值
    uint16_t raw = (data[0] << 8) | data[1];
    lux = raw / 1.2;
    
    // 过滤异常值
    if(lux < 0 || lux > 10000) lux = 0.0;
    //printf("BH1750 读取成功：lux=%.1f\r\n", lux);
    
    return lux;
}

// 核心：按指定帧格式发送光照数据（修正校验和计算）
void BH1750_Send_Light_Frame(void)
{
    uint8_t frame[9] = {0}; // 固定9字节数据帧
    uint16_t myAddr = 0x0002; // 终端地址
    float lux_float = BH1750_ReadLux(); // 先读取光照值
    uint16_t lux_uint = (uint16_t)lux_float; // 转为uint16_t

    // 1. 填充帧头
    frame[0] = 0xAA;

    // 2. 填充终端地址（高8位、低8位）
    frame[1] = (myAddr >> 8) & 0xFF; // 0x00
    frame[2] = myAddr & 0xFF;        // 0x02

    // 3. 填充CMD、LEN
    frame[3] = 0x02; // CMD：光照数据
    frame[4] = 0x02; // LEN：2字节

    // 4. 填充光照值（高8位、低8位）
    frame[5] = (lux_uint >> 8) & 0xFF; // 光照高8位
    frame[6] = lux_uint & 0xFF;        // 光照低8位

    // 5. 【关键】计算校验和（字节1 ~ 字节6 累加）
    uint8_t check_sum = 0;
    for(uint8_t i = 1; i <= 6; i++)
    {
        check_sum += frame[i];
    }
    frame[7] = check_sum; // 填入校验和

    // 6. 填充帧尾
    frame[8] = 0x55;

    // 7. 发送数据帧（核对你的串口句柄）
    HAL_UART_Transmit(&huart1, frame, 9, 100); // 若句柄是&huart1，直接用这个
}
