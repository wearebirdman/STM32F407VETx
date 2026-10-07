#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

/* ========== 定时器配置 ========== */
/* 定时器句柄（CubeMX生成的全局变量名）
 * 需在CubeMX中配置TIMx为PWM输出模式，50Hz（20ms周期）
 * 例如TIM3: Prescaler=83, Period=19999 (84MHz/(83+1)/20000 = 50Hz) */
#define SERVO_TIMX          htim3

/* 定时器参数（用于脉宽→比较值换算）
 * 50Hz时Period=19999，对应20000us，即1us = 1个计数 */
#define SERVO_TIM_PERIOD    19999
#define SERVO_PERIOD_US     20000  /* PWM周期时长（us，50Hz） */

/* ========== 舵机配置 ========== */
#define SERVO_NUM           2     /* 舵机数量 */

/* 舵机ID定义（从0开始） */
#define SERVO_ID_0          0
#define SERVO_ID_1          1
/* #define SERVO_ID_2       2 */
/* #define SERVO_ID_3       3 */

/* 每个舵机对应的定时器通道（根据实际接线修改，通道号必须与CubeMX配置一致） */
#define SERVO0_CH           TIM_CHANNEL_1
#define SERVO1_CH           TIM_CHANNEL_2
/* #define SERVO2_CH         TIM_CHANNEL_3 */
/* #define SERVO3_CH         TIM_CHANNEL_4 */

/* 舵机参数 */
#define SERVO_PULSE_MIN     500   /* 0°对应脉宽（us） */
#define SERVO_PULSE_MAX     2500  /* 180°对应脉宽（us） */
#define SERVO_ANGLE_MAX     180   /* 最大角度（度） */

/* 函数接口 */
void Servo_Init(void);     /* 初始化舵机PWM（启动所有通道的PWM输出） */

/* 设置指定舵机的角度
 * id: 舵机编号（0 ~ SERVO_NUM-1）
 * angle: 角度（0 ~ SERVO_ANGLE_MAX）
 * 返回: 0=成功, 1=失败（id越界或角度越界） */
uint8_t Servo_SetAngle(uint8_t id, int16_t angle);

/* 获取指定舵机当前角度
 * 返回当前角度，id越界时返回 -1 */
int16_t Servo_GetAngle(uint8_t id);

#endif /* __SERVO_H */
