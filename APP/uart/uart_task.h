#ifndef __UART_TASK_H__
#define __UART_TASK_H__

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* 函数接口 */
void Uart_Send(void *argument);         /* TX 属主任务：出队→发硬件→镜像 [Tx] 上屏（uart_task.c） */
void Uart_Recv(void *argument);         /* RX 任务：出队→按 \n 组行→镜像 [Rx] 上屏（uart_task.c） */
void Uart_Printf(const char *fmt, ...); /* printf 风格发送：格式化入队，由 Uart_Send 统一发送（满则丢；收发任务内禁调） */

#endif /* __UART_TASK_H__ */
