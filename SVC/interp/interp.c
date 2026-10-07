#include "interp.h"

/* 线性插值:t=0返回a, t=1返回b, 0<t<1返回中间值 */
float Interp_Linear(float a, float b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return a + (b - a) * t;
}

/* 向target逼近一步，步长不超过max_step */
float Interp_Step(float current, float target, float max_step)
{
    float diff = target - current;

    if (max_step < 0.0f)
        max_step = -max_step;

    if (diff > max_step)
        return current + max_step;
    else if (diff < -max_step)
        return current - max_step;
    else
        return target;   /* 距离小于一步，直接到达 */
}

/* 对count个轴同时做平滑步进插值
 * 返回1表示所有轴都已到达目标，0表示仍在运动中 */
uint8_t Interp_StepMulti(float *current, const float *target,
                         float *out, uint8_t count, float max_step)
{
    uint8_t all_done = 1;

    for (uint8_t i = 0; i < count; i++)
    {
        out[i] = Interp_Step(current[i], target[i], max_step);
        if (out[i] != target[i])
            all_done = 0;
    }

    return all_done;
}
