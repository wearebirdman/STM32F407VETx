#ifndef __BL_CONFIG_H
#define __BL_CONFIG_H

#include "main.h"
#include "bl_param.h"

/* Flash 分区（STM32F407VETx 512KB Flash）
 *   Boot:     0x08000000 ~ 0x0800FFFF (64KB,  扇区0~3)
 *   Param:    0x08010000 ~ 0x0801FFFF (64KB,  扇区4,  升级标志/固件头)
 *   APP:      0x08020000 ~ 0x0803FFFF (128KB, 扇区5)
 *   Download: 0x08040000 ~ 0x0807FFFF (256KB, 扇区6~7, Ymodem暂存区)
 * 暂存区(256KB) >= APP(128KB)，可容纳最大固件 */
#define BL_BOOT_BASE            0x08000000UL
#define BL_BOOT_SIZE            (64UL * 1024UL)

#define BL_APP_BASE             0x08020000UL
#define BL_APP_SIZE             (128UL * 1024UL)
#define BL_APP_END              (BL_APP_BASE + BL_APP_SIZE)

#define BL_DL_BASE              BL_APP_END
#define BL_DL_SIZE              (256UL * 1024UL)
#define BL_DL_END               (BL_DL_BASE + BL_DL_SIZE)

#define BL_FLASH_END            BL_DL_END

#define BL_RAM_BASE             0x20000000UL
#define BL_RAM_SIZE             (128UL * 1024UL)

/* 升级进度回调（UI层注入）
 *   received = 已处理字节，total = 固件总长（未知时为0） */
typedef void (*BlUpdProgress_t)(uint32_t received, uint32_t total, void *ctx);

/* 通用工具宏 */
#define BL_ALIGN4(x)            (((x) + 3U) & ~3UL)
#ifndef BL_MIN
#define BL_MIN(a, b)            (((a) < (b)) ? (a) : (b))
#endif

/* 搬运/校验分块缓冲大小 */
#define BL_DL_BUF_SIZE          4096UL

/* 模块开关：1=使用STM32硬件CRC外设，0=软件查表 */
#define BL_USE_HW_CRC           1

/* 串口Ymodem等待发送方的最大时长（毫秒） */
#define BL_UART_WAIT_MS         30000UL

/* 升级状态枚举（各更新模块共用） */
typedef enum
{
    BL_UPD_OK = 0,          /* 成功 */
    BL_UPD_ERR_OPEN,        /* 打不开固件来源 */
    BL_UPD_ERR_HEADER,      /* 固件头非法 */
    BL_UPD_ERR_SIZE,        /* 数据长度与固件头不符 */
    BL_UPD_ERR_CRC,         /* CRC32校验失败 */
    BL_UPD_ERR_FLASH,       /* Flash擦/写失败 */
    BL_UPD_ERR_ABORT,       /* 发送方中止 */
    BL_UPD_ERR_TIMEOUT,     /* 接收超时 */
    BL_UPD_ERR_PROTO,       /* 传输协议错误 */
    BL_UPD_ERR_PARAM,       /* 参数区读写失败 */
} BlUpdStatus_t;

#endif /* __BL_CONFIG_H */
