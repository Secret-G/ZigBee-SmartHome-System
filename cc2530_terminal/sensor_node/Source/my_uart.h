#ifndef MY_UART_H
#define MY_UART_H
#include "hal_uart.h"

void UART_Init(void);
void UART_Send(uint8 *buf, uint16 len);
#endif // MY_UART_H