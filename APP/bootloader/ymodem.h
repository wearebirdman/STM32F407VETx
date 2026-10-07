#ifndef __YMODEM_H
#define __YMODEM_H

#include "main.h"

/* Ymodem错误码 */
typedef enum
{
    YM_OK = 0,
    YM_ERR_TIMEOUT,   /* 等待超时（发送方无响应） */
    YM_ERR_ABORT,     /* 发送方主动中止（CAN） */
    YM_ERR_SIZE,      /* 数据超出Download区容量 */
    YM_ERR_PROTO,     /* 协议错误（帧校验/块号错误后多次重试仍失败） */
} YmErr_t;

/* 进度回调：received=已收字节，total=固件总长（发送方未提供则为0） */
typedef void (*YmProgress_t)(uint32_t received, uint32_t total);

/* 等待发送方期间的"取消"检查钩子；返回非0则中止接收 */
typedef uint8_t (*YmAbortFn_t)(void);

/* 设置取消检查钩子 */
void Ymodem_SetAbortCheck(YmAbortFn_t fn);

/* 设置等待发送方的最大时长（毫秒） */
void Ymodem_SetWaitMs(uint32_t ms);

/* 执行一次Ymodem接收会话
 *   prog        进度回调（可传0）
 *   p_received  返回实际收到的文件字节数 */
YmErr_t Ymodem_RecvFile(YmProgress_t prog, uint32_t *p_received);

#endif /* __YMODEM_H */
