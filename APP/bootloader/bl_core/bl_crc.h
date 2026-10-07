#ifndef __BL_CRC_H
#define __BL_CRC_H

#include "main.h"

/* CRC-32/MPEG-2 计算
 * 算法（与STM32F4硬件CRC外设默认配置一致）：
 *   多项式0x04C11DB7，初值0xFFFFFFFF，输入输出均不反转，无终值异或
 *   字节流按MSB-first顺序计算，32位字按大端字节序喂入硬件
 * 提供流式（分块）与一次性两种接口 */

void BlCrc32_Reset(void);                                 /* 开始新一轮 */
void BlCrc32_Update(const uint8_t *data, uint32_t len);   /* 喂入数据（任意长度/分块） */
uint32_t BlCrc32_Result(void);                            /* 取最终结果 */

uint32_t BlCrc32(const uint8_t *data, uint32_t len);      /* 一次性便捷接口 */

#endif /* __BL_CRC_H */
