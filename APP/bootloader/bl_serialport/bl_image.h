#ifndef __BL_IMAGE_H
#define __BL_IMAGE_H

#include "main.h"
#include "bl_config.h"
#include "bl_flash.h"

/* 固件头合法性：magic正确且固件体长度不超过APP区 */
uint8_t BlImage_ValidHeader(const BlFwHeader_t *hdr);

/* 完整文件长度 = 16B头 + align4(fw_size) */
uint32_t BlImage_StoreLen(const BlFwHeader_t *hdr);

/* 校验暂存区固件体（含补齐字节）的CRC32，body_base指向固件体起始 */
uint8_t BlImage_VerifyBody(uint32_t body_base, uint32_t fw_size, uint32_t expect_crc);

/* 擦除APP区并把固件体（align4长度）从暂存区搬运到APP区 */
BlFlashErr_t BlImage_ProgramFrom(uint32_t src_base, const BlFwHeader_t *hdr);

#endif /* __BL_IMAGE_H */
