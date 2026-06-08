#ifndef __LED_H
#define __LED_H

#include "main.h"
#include "tim.h"


#define LUX_MIN       10     // 建议别从0开始，避免极端低光误判
#define LUX_MAX       2500    // BH1750室内常见上限，1000也行
#define PWM_MAX_VAL   999
#define HYSTERESIS    5     // 迟滞带，防止抖动

#define OFF_LUX  2300
#define ON_LUX   1800


// 红灯闪烁的亮度值（共阳极：0=最亮，999=灭）
#define RED_ON  0       // 红灯亮
#define RED_OFF 999     // 红灯灭
#define NO_COLOR 999    // 绿/蓝灭

// RGB开关状态枚举（极简定义）
typedef enum
{
    RGB_OFF = 0,  // 关闭
    RGB_ON  = 1   // 开启
} RGB_State;

// ************************ 全局声明 ************************
// RGB状态全局变量（外部可访问，按需使用）
extern RGB_State g_RGB_State;

extern uint8_t g_Auto_Mode;

extern uint8_t g_Smoke_Alarm;

// ************************ 极简调用函数（主推） ************************
/**
 * @brief  RGB开灯（直接亮暖色，一行调用）
 * @param  无
 * @retval 无
 */
void RGB_On(void);

/**
 * @brief  RGB关灯（直接全灭，一行调用）
 * @param  无
 * @retval 无
 */
void RGB_Off(void);

// ************************ 基础初始化函数 ************************
/**
 * @brief  RGB PWM初始化（启动TIM2通道，初始全灭）
 * @param  无
 * @retval 无
 */
void RGB_PWM_Init(void);

// ************************ 兼容旧逻辑函数（保留） ************************
/**
 * @brief  设置RGB状态（适配旧代码/上位机指令）
 * @param  state: RGB_OFF(0) 关闭，RGB_ON(1) 开启
 * @retval 无
 */
void RGB_Set_State(RGB_State state);

/**
 * @brief  打开RGB（等同于RGB_On()）
 * @param  无
 * @retval 无
 */
void RGB_Turn_On(void);

/**
 * @brief  关闭RGB（等同于RGB_Off()）
 * @param  无
 * @retval 无
 */
void RGB_Turn_Off(void);

// ************************ 扩展功能函数（按需使用） ************************
/**
 * @brief  直接设置RGB颜色（共阳极：0=亮，999=灭）
 * @param  r: 红色占空比(0~PWM_MAX_VAL)
 * @param  g: 绿色占空比(0~PWM_MAX_VAL)
 * @param  b: 蓝色占空比(0~PWM_MAX_VAL)
 * @retval 无
 */
void RGB_Set_Color(uint16_t r, uint16_t g, uint16_t b);

/**
 * @brief  根据光照值自动切换RGB颜色
 * @param  lux: 光照值（float类型）
 * @retval 无
 */
void RGB_Set_By_Lux(float lux);

/**
 * @brief  设置可调色温的暖色
 * @param  warm_level: 暖色等级(0~100)，0最暖，100偏白
 * @retval 无
 */
void RGB_Set_Adjustable_Warm(uint8_t warm_level);

void RGB_Set_Warm_By_Lux(float lux);

void RGB_Fade_To_Target(void);

void RGB_Update_Target_By_Lux(float lux);

#endif /* __LED_H */
