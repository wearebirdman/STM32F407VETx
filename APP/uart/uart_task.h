#ifndef __UART_TASK_H__
#define __UART_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* 串口接收消息：ISR把一帧打包后通过队列传给处理任务 */
#define UART_RX_MSG_MAX  64       /* 单帧最大长度（M 160 140\n 等远小于此） */

typedef struct {
    uint8_t  data[UART_RX_MSG_MAX];
    uint16_t len;
} UartRxMsg_t;

/* 由freertos.c创建的USART1接收队列 */
extern osMessageQueueId_t q_Uart1RxMsgHandle;

/* 函数接口 */
void Uart_Send(void *argument);         /* TX属主任务 */
void Uart_Recv(void *argument);         /* RX任务：出队→解析云台协议→回包 */
void Uart_Printf(const char *fmt, ...); /* printf风格发送 */

#endif /* __UART_TASK_H__ */
