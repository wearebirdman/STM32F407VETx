#ifndef __STEPMOTOR_H
#define __STEPMOTOR_H

#include "main.h"
#include "stepmotor_config.h"

/* 电机方向 */
typedef enum
{
    STEPMOTOR_DIR_STOP = 0,   /* 停止 */
    STEPMOTOR_DIR_CW,         /* 顺时针 */
    STEPMOTOR_DIR_CCW         /* 逆时针 */
} StepMotorDir_t;

/* 电机状态 */
typedef struct
{
    StepMotorDir_t dir;       /* 当前方向 */
    uint8_t step;             /* 当前步序（0~7） */
} StepMotor_t;

/* GPIO配置在CubeMX中完成，此处仅设置初始状态 */
void StepMotor_Init(StepMotor_t *motor);

/* 设置当前步序（0~7） */
void StepMotor_SetStep(StepMotor_t *motor, uint8_t step);

/* 转动指定步数
 * motor: 电机实例
 * direction: 方向（CW/CCW）
 * steps: 步数
 * delay_ms: 每步间隔（毫秒），值越小转速越快
 * 注意: 阻塞式调用，FreeRTOS下会阻塞当前任务 */
void StepMotor_RunSteps(StepMotor_t *motor, StepMotorDir_t direction,
                        uint32_t steps, uint16_t delay_ms);

/* 转动指定角度
 * motor: 电机实例
 * direction: 方向（CW/CCW）
 * angle: 角度（度）
 * delay_ms: 每步间隔（毫秒） */
void StepMotor_RunAngle(StepMotor_t *motor, StepMotorDir_t direction,
                        float angle, uint16_t delay_ms);

/* 立即停止（断电） */
void StepMotor_Stop(StepMotor_t *motor);

/* 获取当前步序 */
uint8_t StepMotor_GetStep(StepMotor_t *motor);

#endif /* __STEPMOTOR_H */
