#ifndef __SERVO_CONFIG_H
#define __SERVO_CONFIG_H

#include "main.h"

/* 定时器句柄（CubeMX生成的全局变量名）
 * 需在CubeMX中配置TIMx为PWM输出模式，50Hz（20ms周期）
 * 例如TIM3: Prescaler=83, Period=19999 (84MHz/(83+1)/20000 = 50Hz) */
#define SERVO_TIMX          htim3

/* 舵机数量 */
#define SERVO_NUM           2

/* 每个舵机对应的定时器通道
 * 根据实际接线修改，通道号必须与CubeMX配置一致 */
#define SERVO0_CH           TIM_CHANNEL_1
#define SERVO1_CH           TIM_CHANNEL_2
/* #define SERVO2_CH         TIM_CHANNEL_3 */
/* #define SERVO3_CH         TIM_CHANNEL_4 */

/* 舵机参数 */
#define SERVO_PULSE_MIN     500     /* 0°对应脉宽（us） */
#define SERVO_PULSE_MAX     2500    /* 180°对应脉宽（us） */
#define SERVO_ANGLE_MAX     180     /* 最大角度（度） */

/* 定时器参数（用于脉宽→比较值换算）
 * 50Hz时Period=19999，对应20000us，即1us = 1个计数 */
#define SERVO_TIM_PERIOD    19999

#endif /* __SERVO_CONFIG_H */
