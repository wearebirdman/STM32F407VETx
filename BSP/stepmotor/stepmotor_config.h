#ifndef __STEPMOTOR_CONFIG_H
#define __STEPMOTOR_CONFIG_H

#include "main.h"

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

#endif /* __STEPMOTOR_CONFIG_H */
