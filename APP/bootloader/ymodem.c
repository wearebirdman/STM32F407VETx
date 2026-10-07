#include "ymodem.h"
#include "bl_config.h"
#include "bl_flash.h"
#include "usart.h"
#include "stm32f4xx_hal.h"

/* Ymodem协议控制字符 */
#define YM_SOH         0x01U
#define YM_STX         0x02U
#define YM_EOT         0x04U
#define YM_ACK         0x06U
#define YM_NAK         0x15U
#define YM_CAN         0x18U
#define YM_CRC         0x43U   /* 'C' */
#define YM_PKT0_LEN    128U
#define YM_BLOCK_MAX   1024U
#define YM_POLL_MS     20U     /* 等待循环轮询间隔 */
#define YM_RX_TIMEOUT  3000U
#define YM_ERR_RETRY   10U     /* 单个块连续错误上限 */

static uint8_t s_block[YM_BLOCK_MAX];

static YmAbortFn_t s_abort_check;
static uint32_t s_wait_ms = 60000U;   /* 等待发送方最大时长（毫秒） */

/* 设置取消检查钩子 */
void Ymodem_SetAbortCheck(YmAbortFn_t fn)
{
    s_abort_check = fn;
}

/* 设置等待发送方的最大时长 */
void Ymodem_SetWaitMs(uint32_t ms)
{
    s_wait_ms = ms;
}

/* 发送单字节 */
static uint8_t Ymodem_Put(uint8_t b)
{
    return (HAL_UART_Transmit(&huart1, &b, 1U, 100U) == HAL_OK);
}

/* 接收单字节（带超时） */
static uint8_t Ymodem_Get(uint8_t *b, uint32_t timeout)
{
    return (HAL_UART_Receive(&huart1, b, 1U, timeout) == HAL_OK);
}

/* 清空接收缓冲残留 */
static void Ymodem_FlushRx(void)
{
    uint8_t b;
    while (Ymodem_Get(&b, 50U))
    {
    }
}

/* CRC16 (XMODEM: poly 0x1021, init 0) */
static uint16_t Ymodem_Crc16(const uint8_t *p, uint32_t len)
{
    uint16_t crc = 0U;
    uint32_t i, j;

    for (i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)(p[i] << 8);
        for (j = 0U; j < 8U; j++)
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
    return crc;
}

/* 读取一帧完整块数据，返回块类型(0xFF=失败) */
static uint8_t Ymodem_ReadBlock(uint8_t type, uint8_t *blk, uint16_t *dlen)
{
    uint8_t cblk, crcH, crcL;
    uint16_t len = (type == YM_SOH) ? 128U : 1024U;

    if (!Ymodem_Get(blk, YM_RX_TIMEOUT) || !Ymodem_Get(&cblk, YM_RX_TIMEOUT))
        return 0xFFU;
    if (HAL_UART_Receive(&huart1, s_block, len, YM_RX_TIMEOUT) != HAL_OK)
        return 0xFFU;
    if (!Ymodem_Get(&crcH, YM_RX_TIMEOUT) || !Ymodem_Get(&crcL, YM_RX_TIMEOUT))
        return 0xFFU;

    if ((*blk ^ cblk) != 0xFFU)
        return 0xFFU;                       /* 块号校验失败 */

    if (Ymodem_Crc16(s_block, len) != (uint16_t)((crcH << 8) | crcL))
        return 0xFFU;                       /* CRC16校验失败 */

    *dlen = len;
    return type;
}

/* 解析文件名包中的文件大小字符串 */
static uint32_t Ymodem_ParseSize(const uint8_t *pkt)
{
    uint32_t i = 0U, size = 0U;

    while ((i < YM_PKT0_LEN) && (pkt[i] != 0U))
        i++;
    i++;                                    /* 跳过NUL */
    while ((i < YM_PKT0_LEN) && (pkt[i] >= '0') && (pkt[i] <= '9'))
    {
        size = size * 10U + (uint32_t)(pkt[i] - '0');
        i++;
    }
    return size;
}

/* 是否为空文件包（文件名包或批结束包） */
static uint8_t Ymodem_IsNullPkt(const uint8_t *pkt)
{
    return (pkt[0] == 0U);
}

/* 执行一次Ymodem接收会话
 * 流程：发送'C' → 收文件名包(块0) → ACK+'C' → 收数据块并逐块校验CRC16
 *       → 遇EOT收尾 → 数据写入Download区（BL_DL_BASE起） */
YmErr_t Ymodem_RecvFile(YmProgress_t prog, uint32_t *p_received)
{
    uint8_t b, blk;
    uint16_t dlen;
    uint32_t received = 0U, total = 0U;
    uint8_t blknum = 1U;
    uint8_t retry = 0U;
    uint8_t r;

    Ymodem_FlushRx();

    /* 阶段A：文件名包（块0）；等待发送方（最长s_wait_ms，按秒倒计时） */
    uint32_t wait_start = HAL_GetTick();
    uint32_t last_sec = 0xFFFFFFFFU;
    Ymodem_Put(YM_CRC);
    while (1)
    {
        if (s_abort_check && s_abort_check())
        {
            if (p_received) *p_received = 0U;
            return YM_ERR_ABORT;            /* 用户取消 */
        }
        uint32_t elapsed = HAL_GetTick() - wait_start;
        if (elapsed >= s_wait_ms)           /* 倒计时结束仍未收到包 → 超时 */
        {
            if (p_received) *p_received = 0U;
            if (prog) prog(0U, (s_wait_ms / 1000U));
            return YM_ERR_TIMEOUT;
        }
        /* 每变化1秒上报一次剩余秒数 */
        uint32_t remain = (s_wait_ms - elapsed) / 1000U;
        if (remain != last_sec)
        {
            last_sec = remain;
            if (prog) prog(remain, (s_wait_ms / 1000U));
        }
        if (!Ymodem_Get(&b, YM_POLL_MS))
            continue;                       /* 无数据：继续等待 */
        if (b == YM_CAN)
        {
            Ymodem_Put(YM_CAN); Ymodem_Put(YM_CAN);
            return YM_ERR_ABORT;
        }
        if ((b == YM_SOH) || (b == YM_STX))
        {
            r = Ymodem_ReadBlock(b, &blk, &dlen);
            if ((r != 0xFFU) && (blk == 0U))
            {
                total = Ymodem_ParseSize(s_block);
                received = 0U;
                if (!Ymodem_Put(YM_ACK))
                    return YM_ERR_ABORT;
                break;                      /* 进入数据阶段 */
            }
            Ymodem_Put(YM_NAK);             /* 坏包：请求重发 */
            continue;
        }
        Ymodem_Put(YM_CRC);                 /* 其它字节：再次请求 */
    }

    /* 擦除Download区 */
    if (BlFlash_EraseRegion(BL_DL_BASE, BL_DL_SIZE) != BL_FLASH_OK)
        return YM_ERR_PROTO;

    Ymodem_Put(YM_CRC);                     /* 请求数据块 */

    /* 阶段B：数据块；块间超时YM_RX_TIMEOUT */
    uint32_t blk_start = HAL_GetTick();
    while (1)
    {
        if (s_abort_check && s_abort_check())
            return YM_ERR_ABORT;            /* 用户取消 */
        if (!Ymodem_Get(&b, YM_POLL_MS))
        {
            if ((HAL_GetTick() - blk_start) >= YM_RX_TIMEOUT)
                return YM_ERR_TIMEOUT;      /* 块间等待超时 */
            continue;
        }
        blk_start = HAL_GetTick();          /* 收到任意字节 → 重置块间计时 */

        if (b == YM_EOT)
            break;                          /* 进入收尾 */

        if (b == YM_CAN)
        {
            Ymodem_Put(YM_CAN); Ymodem_Put(YM_CAN);
            return YM_ERR_ABORT;
        }

        if ((b == YM_SOH) || (b == YM_STX))
        {
            r = Ymodem_ReadBlock(b, &blk, &dlen);
            if (r == 0xFFU)
            {
                if (++retry > YM_ERR_RETRY)
                    return YM_ERR_PROTO;
                Ymodem_Put(YM_NAK);
                continue;
            }
            if (blk != blknum)              /* 块号不符：请求重发 */
            {
                if (++retry > YM_ERR_RETRY)
                    return YM_ERR_PROTO;
                Ymodem_Put(YM_NAK);
                continue;
            }
            retry = 0U;

            if ((received + dlen) > BL_DL_SIZE)
            {
                Ymodem_Put(YM_CAN); Ymodem_Put(YM_CAN);
                return YM_ERR_SIZE;
            }
            if (BlFlash_Write(BL_DL_BASE + received, s_block, dlen) != BL_FLASH_OK)
            {
                Ymodem_Put(YM_CAN); Ymodem_Put(YM_CAN);
                return YM_ERR_PROTO;
            }
            received += dlen;
            blknum = (uint8_t)(blk + 1U);
            Ymodem_Put(YM_ACK);
            if (prog)
                prog(received, total);
        }
        /* 其它字节：忽略 */
    }

    /* 阶段C：收尾（双EOT或批结束空包） */
    Ymodem_Put(YM_ACK);
    Ymodem_Put(YM_CRC);
    for (uint8_t i = 0U; i < 3U; i++)
    {
        if (!Ymodem_Get(&b, YM_RX_TIMEOUT))
            break;                          /* 发送方已结束 */
        if (b == YM_EOT)
        {
            Ymodem_Put(YM_ACK);
            break;
        }
        if (b == YM_SOH)
        {
            r = Ymodem_ReadBlock(YM_SOH, &blk, &dlen);
            if ((r != 0xFFU) && (blk == 0U) && Ymodem_IsNullPkt(s_block))
            {
                Ymodem_Put(YM_ACK);
                break;
            }
        }
        else if (b == YM_CAN)
        {
            Ymodem_Put(YM_CAN); Ymodem_Put(YM_CAN);
            return YM_ERR_ABORT;
        }
    }

    if (p_received)
        *p_received = received;
    return YM_OK;
}
