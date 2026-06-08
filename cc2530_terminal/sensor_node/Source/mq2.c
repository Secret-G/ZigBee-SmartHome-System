#include "mq2.h"
#include "ioCC2530.h"
#include "hal_adc.h"

/*
硬件连接假设：
DO  → P1.5
AO  → ADC6
*/

void MQ2_Init(void)
{
    // 设置 P1.5 为普通IO输入（DO）
    P1SEL &= ~0x20;     // 清除功能选择
    P1DIR &= ~0x20;     // 设置为输入
}

uint8 MQ2_GetDO(void)
{
    // 高电平=未检测到
    // 低电平=检测到

    return (P1_5 > 0) ? 0 : 1;
}

uint8 MQ2_GetAO(void)
{
    uint16 adc = 0;
    float percent = 0;

    adc = HalAdcRead(HAL_ADC_CHANNEL_6, HAL_ADC_RESOLUTION_14);

    if(adc >= 8192)
        return 0;

    percent = ((float)adc / 8192.0f) * 100.0f;

    return (uint8)percent;
}
