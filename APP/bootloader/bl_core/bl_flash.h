#ifndef __BL_FLASH_H
#define __BL_FLASH_H

#include "main.h"

/* Flash操作错误码 */
typedef enum
{
    BL_FLASH_OK = 0,
    BL_FLASH_ERR_PARAM,   /* 参数错误（长度/地址未对齐） */
    BL_FLASH_ERR_RANGE,   /* 越界（触碰Bootloader区或超出Flash） */
    BL_FLASH_ERR_ERASE,   /* 擦除失败 */
    BL_FLASH_ERR_WRITE,   /* 写入失败 */
} BlFlashErr_t;

/* 由地址计算所属Flash扇区号，地址非法返回0xFF */
uint8_t BlFlash_SectorIndex(uint32_t addr);

/* 擦除[addr, addr+len)区间（按扇区对齐向上取整），addr必须>=BL_PARAM_BASE */
BlFlashErr_t BlFlash_EraseRegion(uint32_t addr, uint32_t len);

/* 向Flash写入数据（按32位字），要求addr与len均4字节对齐，addr>=BL_PARAM_BASE */
BlFlashErr_t BlFlash_Write(uint32_t addr, const uint8_t *data, uint32_t len);

/* 从Flash读取数据（直接字节拷贝） */
BlFlashErr_t BlFlash_Read(uint32_t addr, uint8_t *buf, uint32_t len);

#endif /* __BL_FLASH_H */
