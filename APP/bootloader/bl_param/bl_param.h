#ifndef __BL_PARAM_H
#define __BL_PARAM_H

#include "main.h"

/* 参数区（内部Flash 扇区4，64KB） */
#define BL_PARAM_BASE           0x08010000UL
#define BL_PARAM_SIZE           (64UL * 1024UL)
#define BL_PARAM_SECTOR_SIZE    (64UL * 1024UL)
#define BL_PARAM_FLASH_SECTOR   4U          /* 扇区号（擦除单元） */
#define BL_PARAM_CMD_OFF        0U          /* 命令字偏移（4B） */
#define BL_PARAM_HDR_OFF        4U          /* 固件头偏移（16B） */

/* 升级命令字 */
#define BL_CMD_UPGRADE          0xA55A5AA5UL   /* APP写入此值请求进入升级 */
#define BL_CMD_NONE             0x00000000UL

/* 固件头（16字节，小端） */
#define BL_FW_MAGIC             0x46424C4FUL   /* 'F''B''L''O' */
#define BL_FW_HEADER_SIZE       16UL

typedef struct {
    uint32_t magic;        /* BL_FW_MAGIC */
    uint32_t fw_size;      /* 固件体实际字节数（不含头，不含补齐） */
    uint32_t fw_crc32;     /* CRC-32/MPEG-2，覆盖固件体+补齐 */
    uint32_t fw_version;   /* 高16位=主版本，次8位=次版本，低8位=修订 */
} __attribute__((packed)) BlFwHeader_t;

#define BL_FW_VER_MAJOR(v)      (((v) >> 16) & 0xFFUL)
#define BL_FW_VER_MINOR(v)      (((v) >> 8) & 0xFFUL)
#define BL_FW_VER_PATCH(v)      ((v) & 0xFFUL)

/* 参数区错误码 */
typedef enum {
    BL_PARAM_OK = 0,
    BL_PARAM_ERR_FLASH,    /* 参数扇区擦/写/校验失败 */
    BL_PARAM_ERR_PARAM,    /* 参数错误 */
} BlParamErr_t;

/* 参数区读写接口 */
BlParamErr_t BlParam_ReadCmd(uint32_t *cmd);                     /* 读取升级命令字 */
BlParamErr_t BlParam_WriteCmd(uint32_t cmd);                     /* 写入升级命令字（保留原固件头） */
BlParamErr_t BlParam_ReadHeader(BlFwHeader_t *hdr);              /* 读取固件头 */
BlParamErr_t BlParam_WriteHeader(const BlFwHeader_t *hdr);       /* 写入固件头（保留原命令字） */

#endif /* __BL_PARAM_H */
