#include "key_task.h"
#include "key.h"

extern osMessageQueueId_t q_KeyMsgHandle;

void key_proc(void *argument)
{
    KeyMsg_t key_msg;

    Key_Init();

    for (;;)
    {
        key_msg = Key_Scan();
        switch (key_msg.key_id) 
        {
            case KEY_WK_ID:
                if (key_msg.event == KEY_EVENT_SHORT_PRESS) 
                {
                    // 处理 WK_UP 按键短按事件
                }
                break;  
            case KEY_1_ID:
                if (key_msg.event == KEY_EVENT_SHORT_PRESS) 
                {
                    
                }
                break;
            case KEY_2_ID:
                if (key_msg.event == KEY_EVENT_SHORT_PRESS) 
                {
                   
                }
                break;
            case KEY_NONE_ID:
                break;
            default:
                break;
        }
        osDelay(10);   // 每 10ms 扫描一次按键
    }
}
