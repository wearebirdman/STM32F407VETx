#ifndef __STEPMOTOR_H
#define __STEPMOTOR_H

#include "main.h"

/* ========== 硬件配置 ========== */
/* 步进电机硬件配置
 * 28BYJ-48 + ULN2003驱动板, 4相5线制, 8拍半步模式 */

/* 引脚配置: PA0~PA3 -> IN1~IN4 */
#define STEPMOTOR_IN1_PIN   GPIO_PIN_0
#define STEPMOTOR_IN1_PORT  GPIOA
#define STEPMOTOR_IN2_PIN   GPIO_PIN_1
#define STEPMOTOR_IN2_PORT  GPIOA
#define STEPMOTOR_IN3_PIN   GPIO_PIN_2
#define STEPMOTOR_IN3_PORT  GPIOA
#define STEPMOTOR_IN4_PIN   GPIO_PIN_3
#define STEPMOTOR_IN4_PORT  GPIOA

/* 28BYJ-48电机参数
 * 步距角: 5.625°/64（减速比1:64）
 * 8拍半步模式下，每步角度 = 5.625 / 8 / 64 ≈ 0.08789°
 * 一圈 ≈ 4096步 */
#define STEPMOTOR_STEP_ANGLE        0.08789f    /* 每步对应输出轴角度（度） */
#define STEPMOTOR_STEPS_PER_REV     4096        /* 输出轴一圈的步数 */

/* 停止后是否断电（1=断电防发热, 0=保持力矩） */
#define STEPMOTOR_POWER_OFF_ON_STOP 1

/* ========== 类型定义 ========== */
/* 电机方向 */
typedef enum {
    STEPMOTOR_DIR_STOP = 0,   /* 停止 */
    STEPMOTOR_DIR_CW,         /* 顺时针 */
    STEPMOTOR_DIR_CCW         /* 逆时针 */
} StepMotorDir_t;

/* 电机状态 */
typedef struct {
    StepMotorDir_t dir;       /* 当前方向 */
    uint8_t step;             /* 当前步序（0~7） */
} StepMotor_t;

/* 函数接口 */
void StepMotor_Init(StepMotor_t *motor);        /* 初始化（GPIO配置在CubeMX中完成，此处仅设初始状态） */
void StepMotor_SetStep(StepMotor_t *motor, uint8_t step);  /* 设置当前步序（0~7） */

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

void StepMotor_Stop(StepMotor_t *motor);        /* 立即停止（断电） */
uint8_t StepMotor_GetStep(StepMotor_t *motor);  /* 获取当前步序 */

#endif /* __STEPMOTOR_H */
