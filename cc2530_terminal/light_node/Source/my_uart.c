#include "my_uart.h"
#include "OSAL.h"
#include "SampleApp.h"
#include "hal_uart.h"

extern uint8 SampleApp_TaskID;

#define FRAME_LEN 9

uint8 uartBuf[32];
uint8 uartLen = 0;

/* ================= UART ????? ================= */
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

    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
}

/* ================= ????????? ================= */
void UART_RxCallBack(uint8 port, uint8 event)
{
    if(event & (HAL_UART_RX_TIMEOUT | HAL_UART_RX_FULL))
    {
        uint8 ch;

        while(Hal_UART_RxBufLen(port))
        {
            HalUARTRead(port, &ch, 1);

            /* ???????????????? */
            if(ch == 0xAA)
            {
                uartLen = 0;
            }

            uartBuf[uartLen++] = ch;

            if(uartLen >= FRAME_LEN)
            {
                if(uartBuf[0] == 0xAA &&
                   uartBuf[FRAME_LEN-1] == 0x55)
                {
                    osal_set_event(SampleApp_TaskID, UART_RX_EVT);
                }

                uartLen = 0;   // ? ????????
            }

            if(uartLen >= 32)
            {
                uartLen = 0;
            }
        }
    }
}