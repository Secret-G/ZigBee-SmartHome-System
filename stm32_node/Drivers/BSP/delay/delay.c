#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

// 校准参数：72MHz下，1us需要的空循环次数（实测值）
static uint32_t us_ticks = 0;  

// 延时初始化：校准空循环参数（仅计算一次）
void delay_init(void)
{
    // 系统时钟72MHz，1个时钟周期=1/72us ≈0.0138us
    // 空循环指令数≈3，所以1us需要 72/3 = 24 次循环
    us_ticks = SystemCoreClock / 1000000 / 3; 
}

// 微秒级延时（空循环，不依赖SysTick，不影响FreeRTOS）
void delay_us(uint32_t us)
{
    uint32_t ticks = us * us_ticks;
    // 空循环（编译器不会优化，加volatile）
    volatile uint32_t i;
    for(i=0; i<ticks; i++);
}

// 毫秒级延时（优先用FreeRTOS的osDelay，让出CPU）
void delay_ms(uint32_t ms)
{
    // FreeRTOS调度器已启动 → 用osDelay（推荐）
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
      //  osDelay(ms);
			
			vTaskDelay(ms);
        return;
    }
    // 裸机模式 → 用空循环
    while(ms--)
    {
        delay_us(1000);
    }
}
