#include "bl_jump.h"
#include "bl_config.h"
#include "stm32f4xx_hal.h"

typedef void (*BlAppEntry_t)(void);

/* 校验APP区首8字节（栈顶+复位向量）是否构成合法固件
 *   - 栈顶必须4字节对齐且落在SRAM范围内
 *   - 复位向量必须带Thumb位（Cortex-M最低位必须为1）
 *   - PC必须在APP区范围内 */
uint8_t Bl_AppValid(void)
{
    uint32_t sp = *(volatile uint32_t *)BL_APP_BASE;
    uint32_t pc = *(volatile uint32_t *)(BL_APP_BASE + 4U);

    if ((sp & 3U) != 0U)
        return 0U;
    if ((sp < BL_RAM_BASE) || (sp > (BL_RAM_BASE + BL_RAM_SIZE)))
        return 0U;

    if ((pc & 1UL) == 0UL)
        return 0U;
    pc &= ~1UL;
    if ((pc <= BL_APP_BASE) || (pc >= BL_APP_END))
        return 0U;
    return 1U;
}

/* 跳转到APP */
void Bl_JumpToApp(void)
{
    uint32_t sp = *(volatile uint32_t *)BL_APP_BASE;
    uint32_t pc = *(volatile uint32_t *)(BL_APP_BASE + 4U);
    BlAppEntry_t entry = (BlAppEntry_t)pc;
    uint32_t i;

    __disable_irq();
    HAL_RCC_DeInit();
    HAL_DeInit();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* 清空所有中断挂起与使能 */
    for (i = 0U; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = BL_APP_BASE;
    __set_MSP(sp);
    __set_CONTROL(0U);
    __ISB();
    __DSB();
    __enable_irq();
    entry();

    /* 若APP意外返回，则驻留Bootloader */
    while (1)
    {
    }
}
