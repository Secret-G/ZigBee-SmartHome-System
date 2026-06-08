#include "led.h"
#include "stdio.h"
#include "cmsis_os.h"
#include "cmd_parse.h"
#include <math.h>

// 全局变量：RGB开关状态（默认开启）
RGB_State g_RGB_State = RGB_ON;  

// 全局变量：自动模式状态（0=关闭，1=开启）
uint8_t g_Auto_Mode = 0;


// 全局变量：烟雾报警状态（0=正常，1=报警）
uint8_t g_Smoke_Alarm = 0;

// 新增：暖色默认参数（可在这统一修改，不用改函数）
#define WARM_R 100    // 暖色红通道占空比
#define WARM_G 200    // 暖色绿通道占空比
#define WARM_B 900    // 暖色蓝通道占空比




// 初始化RGB PWM（仅启动通道，无冗余操作）
void RGB_PWM_Init(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // R(PA0)
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); // B(PA1)
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // G(PA2)

    // 初始全灭（共阳极：999=灭）
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, PWM_MAX_VAL);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWM_MAX_VAL);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM_MAX_VAL);

    printf("RGB PWM初始化完成\r\n");
}



// 保留原有函数（兼容光照调节/上位机指令，不影响极简调用）
void RGB_Set_Color(uint16_t r, uint16_t g, uint16_t b)
{ 
    r = (r > PWM_MAX_VAL) ? PWM_MAX_VAL : r;
    g = (g > PWM_MAX_VAL) ? PWM_MAX_VAL : g;
    b = (b > PWM_MAX_VAL) ? PWM_MAX_VAL : b;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, r);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, b);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, g);
}


void RGB_On(void)
{
    g_RGB_State = RGB_ON;  // 解锁状态
		RGB_Set_Color(WARM_R,WARM_G,WARM_B);
}

void RGB_Off(void)
{
   // g_RGB_State = RGB_OFF; // 锁定状态
		RGB_Set_Color(PWM_MAX_VAL,PWM_MAX_VAL,PWM_MAX_VAL);
}


void RGB_Set_By_Lux(float lux)
{
    if(g_RGB_State == RGB_OFF) 
    {
        printf("RGB关闭，跳过光照调节（lux=%.1f）\r\n", lux);
        return;
    }
    
    if (lux > 500)
		{	
			// 强光 → 暖光调暗（亮度10%）
			RGB_Set_Color(0, 0, 0);
			printf("强光 → 暖光调暗 (lux=%.1f)\r\n", lux);
		}
		else if (lux > 100)
		{
			// 中等光 → 暖光中等亮度（亮度50%）
			RGB_Set_Color(WARM_R * 0.5, WARM_G * 0.5, WARM_B * 0.5);
			printf("中等光 → 暖光中等亮度 (lux=%.1f)\r\n", lux);
		}
		else
		{
			// 弱光 → 暖光最亮（亮度100%）
			RGB_Set_Color(WARM_R, WARM_G, WARM_B);
			printf("弱光 → 暖光最亮 (lux=%.1f)\r\n", lux);
		}
}

// 保留原有状态函数（兼容旧代码，实际用RGB_On/RGB_Off即可）
void RGB_Turn_On(void)  { RGB_On(); }
void RGB_Turn_Off(void) { RGB_Off(); }
void RGB_Set_State(RGB_State state)
{
    state == RGB_OFF ? RGB_Off() : RGB_On();
}

// 可调暖色函数（保留，按需使用）
void RGB_Set_Adjustable_Warm(uint8_t warm_level)
{
    if(g_RGB_State == RGB_OFF) return;
    
    warm_level = (warm_level > 100) ? 100 : warm_level;
    
    uint16_t r = 100 + warm_level * 2;
    uint16_t g = 200 + warm_level * 3;
    uint16_t b = 900 - warm_level * 5;
    
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, r);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, b);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, g);
}


static float last_lux = -1.0f;
static uint8_t forced_off = 0;

void RGB_Set_Warm_By_Lux(float lux)
{
    if (g_RGB_State == RGB_OFF) {
        return;
    }

    if (lux < LUX_MIN) lux = LUX_MIN;
    if (lux > LUX_MAX) lux = LUX_MAX;

    if (!forced_off && lux >= 2300) {
        forced_off = 1;
    } else if (forced_off && lux <= 1800) {
        forced_off = 0;
    }

    if (forced_off) {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, PWM_MAX_VAL);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, PWM_MAX_VAL);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, PWM_MAX_VAL);
        return;
    }

    if (last_lux >= 0.0f && fabsf(lux - last_lux) < HYSTERESIS) {
        return;
    }
    last_lux = lux;

    float bright_coeff = (lux - LUX_MIN) / (float)(LUX_MAX - LUX_MIN);
    if (bright_coeff < 0.0f) bright_coeff = 0.0f;
    if (bright_coeff > 1.0f) bright_coeff = 1.0f;

    bright_coeff = bright_coeff * bright_coeff;

    const uint16_t warm_r_base = 100;
    const uint16_t warm_g_base = 200;
    const uint16_t warm_b_base = 900;

    uint16_t r = warm_r_base + (uint16_t)(bright_coeff * (PWM_MAX_VAL - warm_r_base));
    uint16_t g = warm_g_base + (uint16_t)(bright_coeff * (PWM_MAX_VAL - warm_g_base));
    uint16_t b = warm_b_base + (uint16_t)(bright_coeff * (PWM_MAX_VAL - warm_b_base));

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, r);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, g);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, b);
}


