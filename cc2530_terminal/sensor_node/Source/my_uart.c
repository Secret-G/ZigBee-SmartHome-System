#include "my_uart.h"

void UART_Init(void)
{
    halUARTCfg_t uartConfig;

    //osal_memset(&uartConfig, 0, sizeof(uartConfig));

    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 0;
    uartConfig.rx.maxBufSize        = 64;
    uartConfig.tx.maxBufSize        = 64;
    uartConfig.idleTimeout          = 6;
    uartConfig.intEnable            = FALSE;
    uartConfig.callBackFunc         = NULL;

    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
}
void UART_Send(uint8 *buf, uint16 len)
{
    HalUARTWrite(HAL_UART_PORT_0, buf, len);
}
