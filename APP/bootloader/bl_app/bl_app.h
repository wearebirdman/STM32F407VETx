#ifndef __BL_APP_H
#define __BL_APP_H

#include "main.h"
#include "bl_param.h"

/* 请求进入Bootloader串口升级模式
 * 向参数区写入BL_CMD_UPGRADE命令字，成功后系统复位（本函数不返回）
 * 返回: BL_PARAM_OK以外的值表示写入失败（未复位，可继续运行） */
BlParamErr_t BlApp_RequestUpgrade(void);

#endif /* __BL_APP_H */
