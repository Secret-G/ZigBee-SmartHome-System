#include "beep.h"

// P0.7 控制蜂鸣器

#define BEEP_PIN      P0_7
#define BEEP_DIR      P0DIR
#define BEEP_SEL      P0SEL
#define BEEP_MASK     0x80   // 1000 0000 -> P0.7

void BEEP_Init(void)
{
    BEEP_SEL &= ~BEEP_MASK;   // 普通GPIO
    BEEP_DIR |= BEEP_MASK;    // 设置为输出
    BEEP_PIN = 1;             // 默认关闭（低电平响还是高电平响按你电路决定）
}

void BEEP_On(void)
{
    BEEP_PIN = 0;   // 如果低电平响
}

void BEEP_Off(void)
{
    BEEP_PIN = 1;
}

void BEEP_Toggle(void)
{
    BEEP_PIN = !BEEP_PIN;
}
