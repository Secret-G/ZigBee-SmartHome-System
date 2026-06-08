#include "my_uart.h"
#include "OSAL.h"
#include "SampleApp.h"
#include "hal_uart.h"

extern uint8 SampleApp_TaskID;

uint8 uartBuf[32];
uint8 uartLen = 0;

void UART_Init(void)
{
    halUARTCfg_t uartConfig;

    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 0;
    uartConfig.rx.maxBufSize        = 64;
    uartConfig.tx.maxBufSize        = 64;
    uartConfig.idleTimeout          = 6;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = UART_RxCallBack;

    HalUARTOpen(HAL_UART_PORT_0, &uartConfig); // 必须打开UART
}

void UART_RxCallBack(uint8 port, uint8 event)
{
    if(event & (HAL_UART_RX_TIMEOUT | HAL_UART_RX_FULL))
    {
        uint8 ch;

        while(Hal_UART_RxBufLen(port))
        {
            HalUARTRead(port, &ch, 1);

            // 帧头重置：0xAA开头
            if(ch == 0xAA)
            {
                uartLen = 0;
            }

            // 防止缓冲区溢出
            if(uartLen < 32)
            {
                uartBuf[uartLen++] = ch;
            }

            // 下行8字节帧触发事件（协调器发送下行数据）
            if(uartLen == DOWNLINK_FRAME_LEN)
            {
                if(uartBuf[0] == 0xAA && uartBuf[DOWNLINK_FRAME_LEN-1] == 0x55)
                {
                    osal_set_event(SampleApp_TaskID, UART_RX_EVT);
                }
                // 仅清空无效帧，有效帧处理后再清空（避免提前丢失）
                if(!(uartBuf[0] == 0xAA && uartBuf[DOWNLINK_FRAME_LEN-1] == 0x55))
                {
                    uartLen = 0;
                }
            }

            // 上行9字节帧（若协调器接收终端上行数据）
            if(uartLen == UPLINK_FRAME_LEN)
            {
                if(uartBuf[0] == 0xAA && uartBuf[UPLINK_FRAME_LEN-1] == 0x55)
                {
                    osal_set_event(SampleApp_TaskID, UART_RX_EVT);
                }
                if(!(uartBuf[0] == 0xAA && uartBuf[UPLINK_FRAME_LEN-1] == 0x55))
                {
                    uartLen = 0;
                }
            }

            // 缓冲区溢出保护
            if(uartLen >= 32)
            {
                uartLen = 0;
            }
        }
    }
}