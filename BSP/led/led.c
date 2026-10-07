#include "led.h"

/* ========== LED基本控制 ========== */

/* LED开启函数 */
void Led_On(LedID_t led_id)
{
    switch (led_id)
    {
    case LED_1_ID:
        HAL_GPIO_WritePin(LED_1_PORT, LED_1_PIN, LED_1_ON_LEVEL);
        break;
    default:
        break;
    }
}

/* LED关闭函数 */
void Led_Off(LedID_t led_id)
{
    switch (led_id)
    {
    case LED_1_ID:
        HAL_GPIO_WritePin(LED_1_PORT, LED_1_PIN, LED_1_OFF_LEVEL);
        break;
    default:
        break;
    }
}

/* LED反转函数 */
void Led_Toggle(LedID_t led_id)
{
    switch (led_id)
    {
    case LED_1_ID:
        HAL_GPIO_TogglePin(LED_1_PORT, LED_1_PIN);
        break;
    default:
        break;
    }
}

/* ========== LED请求刷新 ========== */

/* LED刷新函数 */
void Led_Refresh(LedReq_t led_req)
{
    switch (led_req.led_id)
    {
    case LED_1_ID:
        switch (led_req.state)
        {
        case LED_ON:
            Led_On(LED_1_ID);
            break;
        case LED_OFF:
            Led_Off(LED_1_ID);
            break;
        case LED_TOGGLE:
            Led_Toggle(LED_1_ID);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
