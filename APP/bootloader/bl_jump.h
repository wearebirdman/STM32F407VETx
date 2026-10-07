#ifndef __BL_JUMP_H
#define __BL_JUMP_H

#include "main.h"

/* 校验APP区首8字节（栈顶+复位向量）是否构成合法固件，1=合法 0=非法 */
uint8_t Bl_AppValid(void);

/* 跳转到APP
 * 跳转前：关中断、反初始化外设、清NVIC、设置VTOR、重设MSP
 * 跳转前必须重新开中断，否则APP内所有中断会永久静默 */
void Bl_JumpToApp(void);

#endif /* __BL_JUMP_H */
