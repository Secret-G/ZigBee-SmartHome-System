/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bh1750.h"
#include "led.h"
#include "cmd_parse.h" 
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
osThreadId_t uartTaskHandle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ��������ֻ��LED��˸+��ӡ

// 串口解析任务（兼容开关灯+自动模式）
void vUartParseTask(void *pvParameters)
{
    uartTaskHandle = osThreadGetId(); // 给中断回调的句柄赋值
    Uart1_RxStart(); // 启动串口接收
     
    printf("串口解析任务启动...\r\n");
    
    for(;;)
    {
        // 等待中断的帧就绪标志（0x01）
        osThreadFlagsWait(0x01, osFlagsWaitAll, osWaitForever);
               
        // **************** 核心解析逻辑 ****************
        uint8_t cmd = rx_buf[3];       // 主指令码（固定0x10）
				uint8_t len     = rx_buf[4];
        uint8_t sub_cmd = rx_buf[5];   // 子命令：01=开关灯，02=自动模式
        uint8_t param = rx_buf[6];     // 参数：01=开启，00=关闭
        
        // 主指令码都是0x02，靠子命令区分功能
        if(cmd == 0x10 && len == 0x02)
        {
            // 1. 子命令01：开关灯（原有逻辑）
            if(sub_cmd == 0x01)
            {
                if(param == 0x01)
                {
                    RGB_On();
                }
                else if(param == 0x00)
                {
                    RGB_Off();
                }
            }
            // 2. 子命令02：自动模式（新增逻辑）
            else if(sub_cmd == 0x02)
            {
                if(param == 0x01)
                {
                    g_Auto_Mode = 1; // 标记开启自动模式
                    // 自动模式开启后：RGB保持开启，按光照调节亮度
                    RGB_On(); // 先开灯，再由传感器任务调节亮度
                }
                else if(param == 0x00)
                {
                    g_Auto_Mode = 0; // 标记关闭自动模式
                    // 自动模式关闭后：RGB保持当前开关状态，停止光照调节
                }
            }
						else if(sub_cmd == 0x03)
						{
								if(param == 0x01)
								{
									  g_Smoke_Alarm = 1;
										g_Auto_Mode = 0;
								}
								else if(param == 0x00)
								{
										g_Smoke_Alarm = 0;
									  RGB_Off();
								}
						}
        }
    }
}

void vLEDTask(void *pvParameters)
{
    float lux = 0.0f;    
    uint32_t flash_count = 0; // 闪烁计数（控制500ms亮/灭）
    for(;;)
    {
        // 优先级：烟雾报警 > 自动模式
        if(g_Smoke_Alarm == 1)
        {
            // 复用RGB_Set_Color实现红灯闪烁，无需直接操作寄存器
            flash_count++;
            if(flash_count % 2 == 1) // 奇数：亮红灯
            {
                // 红灯亮（R=0），绿/蓝灭（G/B=999）
                RGB_Set_Color(RED_ON, NO_COLOR, NO_COLOR);
            }
            else // 偶数：灭红灯
            {
                // 所有灯灭（R/G/B=999）
                RGB_Set_Color(RED_OFF, NO_COLOR, NO_COLOR);
            }
            vTaskDelay(500); // 500ms切换一次状态
        }
				
				
        // 无报警：原有自动模式逻辑（继续复用RGB_Set_Warm_By_Lux，内部也调用RGB_Set_Color）
        else if(g_Auto_Mode == 1)
        {
            lux = BH1750_ReadLux(); // 读取光照值
            RGB_Set_Warm_By_Lux(lux); // 内部已调用RGB_Set_Color，无需重复操作
						vTaskDelay(20);
					
					
        }
        else
        {
            // 无报警、无自动模式：保持当前RGB状态，不做操作
            vTaskDelay(1000);
        }
				
    }
}
//// 上传光照帧
//void vSensorTask(void *pvParameters)
//{ 
//    for(;;)
//    {
//       BH1750_Send_Light_Frame();       // 发送光照帧
//      vTaskDelay(1000);                   // FreeRTOS延时1秒
//    }
//}

// 光照采集测试任务：只打印 BH1750 光照值，方便论文截图
void vSensorTask(void *pvParameters)
{
    float lux = 0.0f;
    uint16_t lux_int = 0;

    for(;;)
    {
        // 读取 BH1750 当前光照强度
        lux = BH1750_ReadLux();

        // 防止异常负值影响显示
        if(lux < 0)
        {
            lux = 0;
        }

        // 转成整数，串口显示更清楚
        lux_int = (uint16_t)(lux + 0.5f);

        // 串口输出，方便截图
        printf("BH1750_Lux=%u lx\r\n", lux_int);

        // 每 1 秒采集一次
        vTaskDelay(1000);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	BH1750_Init();
	RGB_PWM_Init();

    xTaskCreate(vLEDTask, "LEDTask", 256, NULL, 2, NULL);
		xTaskCreate(vSensorTask, "vSensorTask", 256, NULL, 2, NULL);
		// 创建串口解析任务
    osThreadAttr_t uartTaskAttr = {
        .name = "UartParseTask",
        .stack_size = 512,
        .priority = (osPriority_t) osPriorityHigh,
    };
    uartTaskHandle = osThreadNew(vUartParseTask, NULL, &uartTaskAttr);
		
		uart_sem_handle = osSemaphoreNew(1, 0, NULL); 
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		printf("����ʧ��\r\n");
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
