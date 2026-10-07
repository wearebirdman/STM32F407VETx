#include "bl_app.h"

/* 请求升级：写参数区命令字 -> 复位，Bootloader上电检测到命令字后进入升级菜单 */
BlParamErr_t BlApp_RequestUpgrade(void)
{
    BlParamErr_t err = BlParam_WriteCmd(BL_CMD_UPGRADE);

    if (err == BL_PARAM_OK)
        NVIC_SystemReset();     /* 正常不返回 */
    return err;
}
