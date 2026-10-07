#include "pid.h"
#include <math.h>   /* fabsf */

/* ===================== 增量式PID ===================== */

/* 增量式PID初始化（所有字段置零，deadband默认设一个小值）
 * 调用后需手动设置kp/ki/kd/output_max，其余可用默认值 */
void PidInc_Init(PidInc_t *pid)
{
    if (pid == NULL)
        return;

    pid->kp           = 0.0f;
    pid->ki           = 0.0f;
    pid->kd           = 0.0f;
    pid->ek           = 0.0f;
    pid->ek1          = 0.0f;
    pid->ek2          = 0.0f;
    pid->output_max   = 0.0f;
    pid->integral_max = 100.0f;    /* 默认较大的积分限幅 */
    pid->deadband     = 2.0f;      /* 默认死区 */
}

/* 增量式PID计算
 * 误差 = actual - target
 * 公式: Δu = Kp·[e(k)-e(k-1)] + Ki·e(k) + Kd·[e(k)-2e(k-1)+e(k-2)] */
float PidInc_Calc(PidInc_t *pid, float target, float actual)
{
    if (pid == NULL)
        return 0.0f;

    float ek = actual - target;

    /* 死区处理:清零误差历史，防止微小抖动通过积分累积放大 */
    if (fabsf(ek) < pid->deadband)
    {
        pid->ek  = 0.0f;
        pid->ek1 = 0.0f;
        pid->ek2 = 0.0f;
        return 0.0f;
    }

    pid->ek = ek;
    float delta = pid->kp * (pid->ek - pid->ek1)
                + pid->ki *  pid->ek
                + pid->kd * (pid->ek - 2.0f * pid->ek1 + pid->ek2);

    /* 输出钳位 */
    if (pid->output_max > 0.0f)
    {
        if (delta > pid->output_max)
            delta = pid->output_max;
        else if (delta < -pid->output_max)
            delta = -pid->output_max;
    }

    /* 更新误差历史 */
    pid->ek2 = pid->ek1;
    pid->ek1 = pid->ek;

    return delta;
}

/* 增量式PID复位（清除误差历史，保留参数） */
void PidInc_Reset(PidInc_t *pid)
{
    if (pid == NULL)
        return;
    pid->ek  = 0.0f;
    pid->ek1 = 0.0f;
    pid->ek2 = 0.0f;
}

/* ===================== 位置式PID ===================== */

/* 位置式PID初始化 */
void PidPos_Init(PidPos_t *pid)
{
    if (pid == NULL)
        return;

    pid->kp           = 0.0f;
    pid->ki           = 0.0f;
    pid->kd           = 0.0f;
    pid->ek           = 0.0f;
    pid->ek1          = 0.0f;
    pid->integral     = 0.0f;
    pid->output_max   = 0.0f;
    pid->integral_max = 100.0f;
    pid->deadband     = 2.0f;
}

/* 位置式PID计算
 * 误差 = actual - target
 * 公式: u = Kp·e(k) + Ki·Σe(i) + Kd·[e(k)-e(k-1)] */
float PidPos_Calc(PidPos_t *pid, float target, float actual)
{
    if (pid == NULL)
        return 0.0f;

    float ek = actual - target;

    /* 死区处理:不累加积分、不更新微分历史 */
    if (fabsf(ek) < pid->deadband)
    {
        pid->ek = 0.0f;
        return 0.0f;
    }

    pid->ek = ek;

    /* 积分累加 & 钳位 */
    pid->integral += ek;
    if (pid->integral_max > 0.0f)
    {
        if (pid->integral > pid->integral_max)
            pid->integral = pid->integral_max;
        else if (pid->integral < -pid->integral_max)
            pid->integral = -pid->integral_max;
    }

    /* 位置式PID核心公式 */
    float output = pid->kp * pid->ek
                 + pid->ki * pid->integral
                 + pid->kd * (pid->ek - pid->ek1);

    /* 输出钳位 */
    if (pid->output_max > 0.0f)
    {
        if (output > pid->output_max)
            output = pid->output_max;
        else if (output < -pid->output_max)
            output = -pid->output_max;
    }

    pid->ek1 = pid->ek;

    return output;
}

/* 位置式PID复位（清除积分和误差历史，保留参数） */
void PidPos_Reset(PidPos_t *pid)
{
    if (pid == NULL)
        return;
    pid->ek       = 0.0f;
    pid->ek1      = 0.0f;
    pid->integral = 0.0f;
}
