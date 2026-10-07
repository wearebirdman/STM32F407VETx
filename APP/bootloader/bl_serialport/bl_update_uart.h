#ifndef __BL_UPDATE_UART_H
#define __BL_UPDATE_UART_H

#include "main.h"
#include "bl_config.h"

/* 执行串口Ymodem升级：接收→暂存Download区→CRC32校验→搬运到APP区
 *   prog  进度回调（UI层注入，可传NULL）
 *   ctx   透传给回调的用户上下文 */
BlUpdStatus_t Bl_UpdateUartStart(BlUpdProgress_t prog, void *ctx);

#endif /* __BL_UPDATE_UART_H */
