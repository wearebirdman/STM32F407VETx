#include "key_task.h"
#include "key.h"

#define KEY_SCAN_PERIOD_MS  10  /* 按键扫描周期（ms） */

/* 按键任务入口 */
void Key_Proc(void *argument)
{
    KeyMsg_t key_msg;

    (void)argument;

    Key_Init();

    for (;;)
    {
        key_msg = Key_Scan();
        switch (key_msg.key_id)
        {
        case KEY_WK_ID:
            if (key_msg.event == KEY_EVENT_SHORT_PRESS)
            {
                /* 处理 WK_UP 按键短按事件 */
            }
            break;
        case KEY_1_ID:
            if (key_msg.event == KEY_EVENT_SHORT_PRESS)
            {
                /* 处理 KEY_1 按键短按事件 */
            }
            break;
        case KEY_2_ID:
            if (key_msg.event == KEY_EVENT_SHORT_PRESS)
            {
                /* 处理 KEY_2 按键短按事件 */
            }
            break;
        case KEY_NONE_ID:
            break;
        default:
            break;
        }
        osDelay(KEY_SCAN_PERIOD_MS);  /* 按扫描周期轮询按键 */
    }
}
