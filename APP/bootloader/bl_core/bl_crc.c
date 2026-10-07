#include "bl_crc.h"
#include "bl_config.h"
#include "stm32f4xx_hal.h"

#if (BL_USE_HW_CRC == 1)
#include "crc.h"   /* extern CRC_HandleTypeDef hcrc */
#endif

/* 软件查表（公共） */
#define BL_CRC32_POLY  0x04C11DB7UL

static uint32_t s_Crc32Tab[256];
static uint8_t  s_Crc32TabOk;

static void BlCrc32_BuildTab(void)
{
    uint32_t i, j, c;

    for (i = 0U; i < 256U; i++)
    {
        c = i << 24;
        for (j = 0U; j < 8U; j++)
            c = (c & 0x80000000UL) ? ((c << 1) ^ BL_CRC32_POLY) : (c << 1);
        s_Crc32Tab[i] = c;
    }
    s_Crc32TabOk = 1U;
}

/* 从给定状态继续计算len字节（用于硬件结尾的1~3字节补齐） */
static uint32_t BlCrc32_SwBlock(uint32_t crc, const uint8_t *p, uint32_t len)
{
    if (!s_Crc32TabOk)
        BlCrc32_BuildTab();
    while (len--)
        crc = (crc << 8) ^ s_Crc32Tab[((crc >> 24) ^ (*p++)) & 0xFFU];
    return crc;
}

/* 流式状态 */
static uint8_t  s_Pend[4];      /* 凑足4字节再喂硬件 */
static uint8_t  s_PendCnt;
static uint8_t  s_Started;

#if (BL_USE_HW_CRC == 0)
static uint32_t s_SwCrc;       /* 软件路径的当前状态 */
#endif

/* 开始新一轮CRC计算 */
void BlCrc32_Reset(void)
{
#if (BL_USE_HW_CRC == 1)
    if (hcrc.Instance == NULL)
        hcrc.Instance = CRC;
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);   /* 外设复位后DR=0xFFFFFFFF，即算法初值 */
#else
    s_SwCrc = 0xFFFFFFFFUL;
#endif
    s_PendCnt = 0U;
    s_Started = 1U;
}

/* 喂入数据（任意长度/分块） */
void BlCrc32_Update(const uint8_t *data, uint32_t len)
{
    if (data == NULL)
        return;
    if (!s_Started)
        BlCrc32_Reset();

#if (BL_USE_HW_CRC == 1)
    while (len--)
    {
        s_Pend[s_PendCnt++] = *data++;
        if (s_PendCnt == 4U)
        {
            uint32_t w = ((uint32_t)s_Pend[0] << 24) | ((uint32_t)s_Pend[1] << 16) |
                         ((uint32_t)s_Pend[2] << 8)  | (uint32_t)s_Pend[3];
            CRC->DR = w;                 /* 每4字节喂一次硬件 */
            s_PendCnt = 0U;
        }
    }
#else
    s_SwCrc = BlCrc32_SwBlock(s_SwCrc, data, len);
#endif
}

/* 取最终结果 */
uint32_t BlCrc32_Result(void)
{
    uint32_t crc;

    if (!s_Started)
        BlCrc32_Reset();

#if (BL_USE_HW_CRC == 1)
    if (s_PendCnt != 0U)
    {
        uint32_t state = CRC->DR;        /* 读出硬件当前状态 */
        crc = BlCrc32_SwBlock(state, s_Pend, s_PendCnt);
        s_PendCnt = 0U;
    }
    else
    {
        crc = CRC->DR;
    }
#else
    crc = s_SwCrc;
#endif

    s_Started = 0U;
    return crc;
}

/* 一次性便捷接口 */
uint32_t BlCrc32(const uint8_t *data, uint32_t len)
{
    BlCrc32_Reset();
    BlCrc32_Update(data, len);
    return BlCrc32_Result();
}
