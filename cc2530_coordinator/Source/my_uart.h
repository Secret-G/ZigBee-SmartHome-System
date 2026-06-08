#ifndef MY_UART_H
#define MY_UART_H
#include "hal_uart.h"

// 明确区分上行/下行帧长度
#define DOWNLINK_FRAME_LEN 8  // 协调器→终端（下行）：8字节
#define UPLINK_FRAME_LEN   9  // 终端→协调器（上行）：9字节

extern uint8 uartBuf[32];
extern uint8 uartLen;

void UART_Init(void);
void UART_Send(uint8 *buf, uint16 len);
void UART_RxCallBack(uint8 port, uint8 event);

#endif // MY_UART_H