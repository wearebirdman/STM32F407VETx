#include "uart_task.h"
#include "usart.h"
#include "servo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* 云台配置（2轴）
 * limits[i] = {min, max} 单位:度 */
#define AXIS_COUNT  2
static const int16_t s_limits[AXIS_COUNT][2] = {{20, 160}, {40, 140}};
static int16_t s_pos[AXIS_COUNT] = {20, 40};   /* 上电归位到各轴下限 */

/* 阻塞发送字符串（115200下短回包<2ms，可接受） */
static void Uart_SendStr(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/* 去掉行尾 \r \n 和空格 */
static void Uart_StripEol(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' '))
        s[--n] = '\0';
}

void Uart_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Uart_SendStr(buf);
}

/* TX属主任务（当前协议用阻塞发送，此任务预留） */
void uart_send(void *argument)
{
    (void)argument;
    for (;;)
    {
        osDelay(100);
    }
}

/* 串口协议处理任务：从接收队列取帧 -> 解析 -> 回包
 * 支持命令:
 *   PING        -> OK PING
 *   S           -> POS pos0 pos1
 *   LIM         -> LIM min0 max0 min1 max1
 *   M a0 a1     -> OK pos0 pos1 / ERR */
void uart_recv(void *argument)
{
    (void)argument;
    UartRxMsg_t msg;
    char line[UART_RX_MSG_MAX];

    for (;;)
    {
        if (osMessageQueueGet(q_UartRxHandle, &msg, NULL, osWaitForever) != osOK)
            continue;

        /* 拷贝并保证 \0 结尾 */
        uint16_t n = msg.len;
        if (n >= UART_RX_MSG_MAX)
            n = UART_RX_MSG_MAX - 1;
        memcpy(line, msg.data, n);
        line[n] = '\0';
        Uart_StripEol(line);
        if (line[0] == '\0')
            continue;                       /* 空帧忽略 */

        char *cmd = strtok(line, " ");
        if (cmd == 0)
            continue;

        if (strcmp(cmd, "PING") == 0)
        {
            Uart_SendStr("OK PING\n");
        }
        else if (strcmp(cmd, "S") == 0)
        {
            char buf[48];
            snprintf(buf, sizeof(buf), "POS %d %d\n", s_pos[0], s_pos[1]);
            Uart_SendStr(buf);
        }
        else if (strcmp(cmd, "LIM") == 0)
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "LIM %d %d %d %d\n",
                     s_limits[0][0], s_limits[0][1], s_limits[1][0], s_limits[1][1]);
            Uart_SendStr(buf);
        }
        else if (strcmp(cmd, "M") == 0)
        {
            int16_t want[AXIS_COUNT + 1];   /* 多收一个用于检测参数过多 */
            uint8_t cnt = 0;
            char *tok;

            while ((tok = strtok(NULL, " ")) != 0 && cnt <= AXIS_COUNT)
            {
                char *end;
                long v = strtol(tok, &end, 10);
                if (*end != '\0')           /* 非整数字符 */
                {
                    Uart_SendStr("ERR 2 BAD_ANGLE\n");
                    goto next_frame;
                }
                want[cnt++] = (int16_t)v;
            }
            if (cnt != AXIS_COUNT)
            {
                Uart_SendStr("ERR 2 AXIS_COUNT\n");
                goto next_frame;
            }
            /* 越界检查：任一轴越界则整体拒绝，位置不变 */
            for (uint8_t i = 0; i < AXIS_COUNT; i++)
            {
                if (want[i] < s_limits[i][0] || want[i] > s_limits[i][1])
                {
                    Uart_SendStr("ERR 1 AXIS_LIMIT\n");
                    goto next_frame;
                }
            }
            /* 合法：驱动舵机并更新位置 */
            for (uint8_t i = 0; i < AXIS_COUNT; i++)
            {
                Servo_SetAngle(i, want[i]);
                s_pos[i] = want[i];
            }

            char buf[48];
            snprintf(buf, sizeof(buf), "OK %d %d\n", s_pos[0], s_pos[1]);
            Uart_SendStr(buf);
        }
        else
        {
            Uart_SendStr("ERR 2 UNKNOWN_CMD\n");
        }

    next_frame:;
    }
}

