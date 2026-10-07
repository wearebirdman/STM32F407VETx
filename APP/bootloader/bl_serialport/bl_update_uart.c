#include "bl_update_uart.h"
#include "bl_image.h"
#include "bl_flash.h"
#include "bl_param.h"
#include "ymodem.h"
#include "key.h"          /* 升级期间轮询KEY1中止 */

static BlUpdProgress_t s_prog;
static void *s_ctx;

/* 透传Ymodem协议层进度到注入的回调 */
static void BlUpdateUart_Prog(uint32_t received, uint32_t total)
{
    if (s_prog)
        s_prog(received, total, s_ctx);
}

/* 等待/传输期间按KEY1短按中止 */
static uint8_t BlUpdateUart_AbortByKey1(void)
{
    KeyMsg_t m = Key_Scan();
    return ((m.key_id == KEY_1_ID) && (m.event == KEY_EVENT_SHORT_PRESS)) ? 1U : 0U;
}

/* 执行串口Ymodem升级 */
BlUpdStatus_t Bl_UpdateUartStart(BlUpdProgress_t prog, void *ctx)
{
    BlFwHeader_t hdr;
    uint32_t received = 0U;
    uint32_t store_len;
    YmErr_t r;
    BlUpdStatus_t st = BL_UPD_OK;

    s_prog = prog;
    s_ctx = ctx;

    /* 等待发送方最长BL_UART_WAIT_MS，期间按KEY1可中止 */
    Ymodem_SetAbortCheck(BlUpdateUart_AbortByKey1);
    Ymodem_SetWaitMs(BL_UART_WAIT_MS);
    r = Ymodem_RecvFile(BlUpdateUart_Prog, &received);
    Ymodem_SetAbortCheck(0);
    Ymodem_SetWaitMs(60000U);              /* 复位为默认值 */
    if (r != YM_OK)
    {
        switch (r)
        {
        case YM_ERR_TIMEOUT: st = BL_UPD_ERR_TIMEOUT; break;
        case YM_ERR_ABORT:   st = BL_UPD_ERR_ABORT;   break;
        case YM_ERR_SIZE:    st = BL_UPD_ERR_SIZE;    break;
        default:             st = BL_UPD_ERR_PROTO;   break;
        }
        return st;
    }

    /* 从Download区读回固件头 */
    if (BlFlash_Read(BL_DL_BASE, (uint8_t *)&hdr, BL_FW_HEADER_SIZE) != BL_FLASH_OK)
        return BL_UPD_ERR_FLASH;
    if (!BlImage_ValidHeader(&hdr))
        return BL_UPD_ERR_HEADER;

    store_len = BlImage_StoreLen(&hdr);
    if (received < store_len)
        return BL_UPD_ERR_SIZE;

    if (!BlImage_VerifyBody(BL_DL_BASE + BL_FW_HEADER_SIZE, hdr.fw_size, hdr.fw_crc32))
        return BL_UPD_ERR_CRC;

    if (BlImage_ProgramFrom(BL_DL_BASE, &hdr) != BL_FLASH_OK)
        return BL_UPD_ERR_FLASH;

    /* 记录新固件信息，清除升级标志 */
    BlParam_WriteHeader(&hdr);
    BlParam_WriteCmd(BL_CMD_NONE);

    return BL_UPD_OK;
}
