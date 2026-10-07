#include "uart_task.h"
#include "usart.h"

/* printf 风格发送：格式化入队，由 Uart_Send 统一发送 */
void Uart_Printf(const char *fmt, ...)
{
    /* 基础工程预留：格式化串口发送实现 */
}

/* UART发送任务入口 */
void Uart_Send(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* 基础工程预留：出队→发硬件 */
    }
}

/* UART接收任务入口 */
void Uart_Recv(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* 基础工程预留：出队→解析处理 */
    }
}
