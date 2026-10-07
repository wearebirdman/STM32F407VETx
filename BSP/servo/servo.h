#ifndef __SERVO_H
#define __SERVO_H

#include "servo_config.h"
#include "main.h"

/* 舵机ID定义（从0开始） */
#define SERVO_ID_0          0
#define SERVO_ID_1          1
/* #define SERVO_ID_2       2 */
/* #define SERVO_ID_3       3 */

/* 初始化舵机PWM（启动所有通道的PWM输出） */
void Servo_Init(void);

/* 设置指定舵机的角度
 * id: 舵机编号（0 ~ SERVO_NUM-1）
 * angle: 角度（0 ~ SERVO_ANGLE_MAX）
 * 返回: 0=成功, 1=失败（id越界或角度越界） */
uint8_t Servo_SetAngle(uint8_t id, int16_t angle);

/* 获取指定舵机当前角度
 * 返回当前角度，id越界时返回 -1 */
int16_t Servo_GetAngle(uint8_t id);

#endif /* __SERVO_H */
