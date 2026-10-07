#ifndef __LED_H
#define __LED_H

#include "main.h"

/* LED定义宏 */
#define LED_1_PORT             GPIOE
#define LED_1_PIN              GPIO_PIN_6
#define LED_1_OFF_LEVEL        GPIO_PIN_RESET
#define LED_1_ON_LEVEL         GPIO_PIN_SET

/* LED ID枚举类型 */
typedef enum {
    LED_1_ID = 1,
} LedID_t;

/* LED状态枚举类型 */
typedef enum {
    LED_OFF = 0,
    LED_ON = 1,
    LED_TOGGLE = 2,
} LedState_t;

/* LED请求结构体 */
typedef struct {
    uint8_t  led_id;
    uint8_t  state;
} LedReq_t;

/* LED函数声明 */
void Led_On(LedID_t led_id);         /* 点亮指定LED */
void Led_Off(LedID_t led_id);        /* 熄灭指定LED */
void Led_Toggle(LedID_t led_id);     /* 反转指定LED */
void Led_Refresh(LedReq_t led_req);  /* 按请求刷新LED */

#endif /* __LED_H */
