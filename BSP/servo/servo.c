#include "servo.h"
#include "tim.h"

/* 舵机通道映射表: 索引 = 舵机ID, 值 = 定时器通道 */
static const uint32_t s_ServoChannel[SERVO_NUM] = {
    SERVO0_CH,
#if SERVO_NUM > 1
    SERVO1_CH,
#endif
#if SERVO_NUM > 2
    SERVO2_CH,
#endif
#if SERVO_NUM > 3
    SERVO3_CH,
#endif
};

/* 每个舵机的当前角度（用于查询） */
static int16_t s_ServoCurrentAngle[SERVO_NUM] = {0};

/* 角度 → PWM比较值换算
 * angle: 0 ~ SERVO_ANGLE_MAX
 * pulse: SERVO_PULSE_MIN ~ SERVO_PULSE_MAX (us)
 * compare = pulse * (SERVO_TIM_PERIOD + 1) / 20000
 * (50Hz时Period=19999, 20000us对应20000计数, 1us = 1计数) */
static uint32_t Servo_AngleToCompare(int16_t angle)
{
    if (angle < 0)
        angle = 0;
    if (angle > SERVO_ANGLE_MAX)
        angle = SERVO_ANGLE_MAX;

    uint32_t pulse = SERVO_PULSE_MIN +
        (uint32_t)(SERVO_PULSE_MAX - SERVO_PULSE_MIN) * angle / SERVO_ANGLE_MAX;

    return pulse * (SERVO_TIM_PERIOD + 1) / SERVO_PERIOD_US;
}

/* 启动所有舵机通道的PWM输出 */
void Servo_Init(void)
{
    for (uint8_t i = 0; i < SERVO_NUM; i++)
    {
        HAL_TIM_PWM_Start(&SERVO_TIMX, s_ServoChannel[i]);
        s_ServoCurrentAngle[i] = 0;
    }
}

/* 设置指定舵机角度 */
uint8_t Servo_SetAngle(uint8_t id, int16_t angle)
{
    if (id >= SERVO_NUM)
        return 1;                       /* id越界 */
    if (angle < 0 || angle > SERVO_ANGLE_MAX)
        return 1;                       /* 角度越界 */

    uint32_t compare = Servo_AngleToCompare(angle);
    __HAL_TIM_SET_COMPARE(&SERVO_TIMX, s_ServoChannel[id], compare);

    s_ServoCurrentAngle[id] = angle;
    return 0;
}

/* 获取指定舵机当前角度 */
int16_t Servo_GetAngle(uint8_t id)
{
    if (id >= SERVO_NUM)
        return -1;
    return s_ServoCurrentAngle[id];
}
