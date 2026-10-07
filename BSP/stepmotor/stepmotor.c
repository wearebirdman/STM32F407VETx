#include "stepmotor.h"

/* 8拍半步通电序列
 * 8拍模式（A-AB-B-BC-C-CD-D-DA），精度比4拍高1倍
 * 每行对应 {IN1, IN2, IN3, IN4} 的电平 */
static const uint8_t s_StepSequence[8][4] = {
    {1, 0, 0, 0},   /* A  */
    {1, 1, 0, 0},   /* AB */
    {0, 1, 0, 0},   /* B  */
    {0, 1, 1, 0},   /* BC */
    {0, 0, 1, 0},   /* C  */
    {0, 0, 1, 1},   /* CD */
    {0, 0, 0, 1},   /* D  */
    {1, 0, 0, 1}    /* DA */
};

/* 初始化 */
void StepMotor_Init(StepMotor_t *motor)
{
    if (motor == NULL)
        return;

    motor->dir  = STEPMOTOR_DIR_STOP;
    motor->step = 0;

    /* 所有相断电 */
    HAL_GPIO_WritePin(STEPMOTOR_IN1_PORT, STEPMOTOR_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN2_PORT, STEPMOTOR_IN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN3_PORT, STEPMOTOR_IN3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN4_PORT, STEPMOTOR_IN4_PIN, GPIO_PIN_RESET);
}

/* 设置步序 */
void StepMotor_SetStep(StepMotor_t *motor, uint8_t step)
{
    if (motor == NULL || step > 7)
        return;

    motor->step = step;

    HAL_GPIO_WritePin(STEPMOTOR_IN1_PORT, STEPMOTOR_IN1_PIN,
                      s_StepSequence[step][0] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN2_PORT, STEPMOTOR_IN2_PIN,
                      s_StepSequence[step][1] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN3_PORT, STEPMOTOR_IN3_PIN,
                      s_StepSequence[step][2] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN4_PORT, STEPMOTOR_IN4_PIN,
                      s_StepSequence[step][3] ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* 转动指定步数 */
void StepMotor_RunSteps(StepMotor_t *motor, StepMotorDir_t direction,
                        uint32_t steps, uint16_t delay_ms)
{
    if (motor == NULL || direction == STEPMOTOR_DIR_STOP || steps == 0)
        return;

    motor->dir = direction;

    for (uint32_t i = 0; i < steps; i++)
    {
        if (direction == STEPMOTOR_DIR_CW)
            motor->step = (motor->step + 1) % 8;   /* 顺时针:步序递增 */
        else
            motor->step = (motor->step + 7) % 8;   /* 逆时针:步序递减 */

        StepMotor_SetStep(motor, motor->step);
        HAL_Delay(delay_ms);
    }

#if STEPMOTOR_POWER_OFF_ON_STOP
    StepMotor_Stop(motor);                        /* 停止后断电防发热 */
#else
    motor->dir = STEPMOTOR_DIR_STOP;
#endif
}

/* 转动指定角度 */
void StepMotor_RunAngle(StepMotor_t *motor, StepMotorDir_t direction,
                        float angle, uint16_t delay_ms)
{
    uint32_t steps = (uint32_t)(angle / STEPMOTOR_STEP_ANGLE + 0.5f);
    StepMotor_RunSteps(motor, direction, steps, delay_ms);
}

/* 立即停止 */
void StepMotor_Stop(StepMotor_t *motor)
{
    if (motor == NULL)
        return;

    motor->dir = STEPMOTOR_DIR_STOP;

    HAL_GPIO_WritePin(STEPMOTOR_IN1_PORT, STEPMOTOR_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN2_PORT, STEPMOTOR_IN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN3_PORT, STEPMOTOR_IN3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPMOTOR_IN4_PORT, STEPMOTOR_IN4_PIN, GPIO_PIN_RESET);
}

/* 获取当前步序 */
uint8_t StepMotor_GetStep(StepMotor_t *motor)
{
    if (motor == NULL)
        return 0;
    return motor->step;
}
