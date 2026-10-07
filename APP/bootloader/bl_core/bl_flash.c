#include "bl_flash.h"
#include "bl_config.h"
#include "stm32f4xx_hal.h"

/* 由地址计算所属Flash扇区号（覆盖整个512KB）
 * 扇区0~3: 16KB each, 扇区4: 64KB, 扇区5~7: 128KB each */
uint8_t BlFlash_SectorIndex(uint32_t addr)
{
    if (addr < BL_PARAM_BASE)
        return (uint8_t)((addr - BL_BOOT_BASE) / (16UL * 1024UL));    /* 扇区 0~3 */
    if (addr < BL_APP_BASE)
        return 4U;                                                      /* 扇区 4（参数区） */
    if (addr < BL_FLASH_END)
        return (uint8_t)(5U + ((addr - BL_APP_BASE) / (128UL * 1024UL))); /* 扇区 5~7 */
    return 0xFFU;
}

/* 擦除[addr, addr+len)区间（按扇区对齐向上取整）
 * 安全约束：addr必须>=BL_PARAM_BASE，禁止触碰Bootloader自身 */
BlFlashErr_t BlFlash_EraseRegion(uint32_t addr, uint32_t len)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_err = 0U;
    uint8_t first, last;

    if (len == 0U)
        return BL_FLASH_OK;
    if ((addr < BL_PARAM_BASE) || ((addr + len) > BL_FLASH_END))
        return BL_FLASH_ERR_RANGE;

    first = BlFlash_SectorIndex(addr);
    last  = BlFlash_SectorIndex(addr + len - 1U);
    if ((first == 0xFFU) || (last == 0xFFU))
        return BL_FLASH_ERR_RANGE;

    HAL_FLASH_Unlock();
    erase.TypeErase     = FLASH_TYPEERASE_SECTORS;
    erase.Sector        = first;
    erase.NbSectors     = (uint32_t)(last - first + 1U);
    erase.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    if (HAL_FLASHEx_Erase(&erase, &sector_err) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return BL_FLASH_ERR_ERASE;
    }
    HAL_FLASH_Lock();
    return BL_FLASH_OK;
}

/* 向Flash写入数据（按32位字）
 * 要求addr与len均4字节对齐，addr>=BL_PARAM_BASE */
BlFlashErr_t BlFlash_Write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    BlFlashErr_t ret = BL_FLASH_OK;
    uint32_t i;

    if (len == 0U)
        return BL_FLASH_OK;
    if ((addr < BL_PARAM_BASE) || ((addr + len) > BL_FLASH_END))
        return BL_FLASH_ERR_RANGE;
    if (((addr & 3U) != 0U) || ((len & 3U) != 0U))
        return BL_FLASH_ERR_PARAM;

    HAL_FLASH_Unlock();
    for (i = 0U; i < len; i += 4U)
    {
        uint32_t word = (uint32_t)data[i] |
                        ((uint32_t)data[i + 1U] << 8) |
                        ((uint32_t)data[i + 2U] << 16) |
                        ((uint32_t)data[i + 3U] << 24);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK)
        {
            ret = BL_FLASH_ERR_WRITE;
            break;
        }
    }
    HAL_FLASH_Lock();
    return ret;
}

/* 从Flash读取数据（直接字节拷贝） */
BlFlashErr_t BlFlash_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)addr;
    uint32_t i;

    if ((addr + len) > BL_FLASH_END)
        return BL_FLASH_ERR_RANGE;
    for (i = 0U; i < len; i++)
        buf[i] = src[i];
    return BL_FLASH_OK;
}
