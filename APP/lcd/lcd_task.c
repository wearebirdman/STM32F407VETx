#include "lcd_task.h"
#include "lcd.h"

/* LCD显示任务入口 */
void Lcd_Disp(void *argument)
{
    (void)argument;

    Lcd_Init(); /* LCD 硬件初始化 */

    for (;;)
    {
        /* 基础工程预留：屏幕内容刷新逻辑 */
    }
}
