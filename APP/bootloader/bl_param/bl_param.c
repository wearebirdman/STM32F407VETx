#include "bl_param.h"
#include "stm32f4xx_hal.h"

#define BL_PARAM_BLOCK_SIZE     (BL_FW_HEADER_SIZE + 4U)   /* 20B = cmd + fw_header */

/* 从4字节小端缓冲加载uint32 */
static uint32_t BlParam_Load32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* 将uint32存入4字节小端缓冲 */
static void BlParam_Store32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
    p[2] = (uint8_t)((v >> 16) & 0xFFU);
    p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

/* 擦除整个参数扇区后写入块，并读回比对 */
static BlParamErr_t BlParam_EraseAndWrite(const uint8_t *blk, uint32_t len)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_err = 0U;
    uint32_t i;

    HAL_FLASH_Unlock();
    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = BL_PARAM_FLASH_SECTOR;
    erase.NbSectors    = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    if (HAL_FLASHEx_Erase(&erase, &sector_err) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return BL_PARAM_ERR_FLASH;
    }

    for (i = 0U; i < len; i += 4U)
    {
        uint32_t w = (uint32_t)blk[i] |
                     ((uint32_t)blk[i + 1U] << 8) |
                     ((uint32_t)blk[i + 2U] << 16) |
                     ((uint32_t)blk[i + 3U] << 24);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, BL_PARAM_BASE + i, w) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return BL_PARAM_ERR_FLASH;
        }
    }
    HAL_FLASH_Lock();

    /* 读回校验 */
    for (i = 0U; i < len; i++)
    {
        if (*(volatile const uint8_t *)(BL_PARAM_BASE + i) != blk[i])
            return BL_PARAM_ERR_FLASH;
    }
    return BL_PARAM_OK;
}

/* 读取升级命令字 */
BlParamErr_t BlParam_ReadCmd(uint32_t *cmd)
{
    uint8_t buf[4];
    uint32_t i;

    if (cmd == 0)
        return BL_PARAM_ERR_PARAM;
    for (i = 0U; i < 4U; i++)
        buf[i] = *(volatile const uint8_t *)(BL_PARAM_BASE + BL_PARAM_CMD_OFF + i);
    *cmd = BlParam_Load32(buf);
    return BL_PARAM_OK;
}

/* 写入升级命令字（保留原固件头） */
BlParamErr_t BlParam_WriteCmd(uint32_t cmd)
{
    uint8_t blk[BL_PARAM_BLOCK_SIZE];
    uint32_t i;

    for (i = 0U; i < BL_PARAM_BLOCK_SIZE; i++)
        blk[i] = *(volatile const uint8_t *)(BL_PARAM_BASE + i);
    BlParam_Store32(blk + BL_PARAM_CMD_OFF, cmd);
    return BlParam_EraseAndWrite(blk, BL_PARAM_BLOCK_SIZE);
}

/* 读取固件头 */
BlParamErr_t BlParam_ReadHeader(BlFwHeader_t *hdr)
{
    uint8_t buf[BL_FW_HEADER_SIZE];
    uint32_t i;

    if (hdr == 0)
        return BL_PARAM_ERR_PARAM;
    for (i = 0U; i < BL_FW_HEADER_SIZE; i++)
        buf[i] = *(volatile const uint8_t *)(BL_PARAM_BASE + BL_PARAM_HDR_OFF + i);
    hdr->magic      = BlParam_Load32(buf);
    hdr->fw_size    = BlParam_Load32(buf + 4U);
    hdr->fw_crc32   = BlParam_Load32(buf + 8U);
    hdr->fw_version = BlParam_Load32(buf + 12U);
    return BL_PARAM_OK;
}

/* 写入固件头（保留原命令字） */
BlParamErr_t BlParam_WriteHeader(const BlFwHeader_t *hdr)
{
    uint8_t blk[BL_PARAM_BLOCK_SIZE];
    uint32_t i;

    if (hdr == 0)
        return BL_PARAM_ERR_PARAM;
    for (i = 0U; i < BL_PARAM_BLOCK_SIZE; i++)
        blk[i] = *(volatile const uint8_t *)(BL_PARAM_BASE + i);
    BlParam_Store32(blk + BL_PARAM_HDR_OFF,         hdr->magic);
    BlParam_Store32(blk + BL_PARAM_HDR_OFF + 4U,    hdr->fw_size);
    BlParam_Store32(blk + BL_PARAM_HDR_OFF + 8U,    hdr->fw_crc32);
    BlParam_Store32(blk + BL_PARAM_HDR_OFF + 12U,   hdr->fw_version);
    return BlParam_EraseAndWrite(blk, BL_PARAM_BLOCK_SIZE);
}
