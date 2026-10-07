#include "bl_image.h"
#include "bl_crc.h"

static uint8_t s_Buf[BL_DL_BUF_SIZE];   /* 分块搬运/校验缓冲 */

/* 固件头合法性检查 */
uint8_t BlImage_ValidHeader(const BlFwHeader_t *hdr)
{
    if (hdr == NULL)
        return 0U;
    if (hdr->magic != BL_FW_MAGIC)
        return 0U;
    if ((hdr->fw_size == 0U) || (hdr->fw_size > BL_APP_SIZE))
        return 0U;
    return 1U;
}

/* 完整文件长度 = 16B头 + align4(fw_size) */
uint32_t BlImage_StoreLen(const BlFwHeader_t *hdr)
{
    return BL_FW_HEADER_SIZE + BL_ALIGN4(hdr->fw_size);
}

/* 校验暂存区固件体（含补齐字节）的CRC32 */
uint8_t BlImage_VerifyBody(uint32_t body_base, uint32_t fw_size, uint32_t expect_crc)
{
    uint32_t body_len = BL_ALIGN4(fw_size);
    uint32_t done = 0U;
    uint32_t chunk;

    BlCrc32_Reset();
    while (done < body_len)
    {
        chunk = BL_MIN(body_len - done, (uint32_t)BL_DL_BUF_SIZE);
        if (BlFlash_Read(body_base + done, s_Buf, chunk) != BL_FLASH_OK)
            return 0U;
        BlCrc32_Update(s_Buf, chunk);
        done += chunk;
    }
    return (BlCrc32_Result() == expect_crc) ? 1U : 0U;
}

/* 擦除APP区并把固件体从暂存区搬运到APP区 */
BlFlashErr_t BlImage_ProgramFrom(uint32_t src_base, const BlFwHeader_t *hdr)
{
    uint32_t body_len = BL_ALIGN4(hdr->fw_size);
    uint32_t src = src_base + BL_FW_HEADER_SIZE;
    uint32_t dst = BL_APP_BASE;
    uint32_t done = 0U;
    uint32_t chunk;
    BlFlashErr_t err;

    if (body_len > BL_APP_SIZE)
        return BL_FLASH_ERR_RANGE;

    err = BlFlash_EraseRegion(BL_APP_BASE, body_len);
    if (err != BL_FLASH_OK)
        return err;

    while (done < body_len)
    {
        chunk = BL_MIN(body_len - done, (uint32_t)BL_DL_BUF_SIZE);
        if (BlFlash_Read(src + done, s_Buf, chunk) != BL_FLASH_OK)
            return BL_FLASH_ERR_PARAM;
        err = BlFlash_Write(dst + done, s_Buf, chunk);
        if (err != BL_FLASH_OK)
            return err;
        done += chunk;
    }
    return BL_FLASH_OK;
}
