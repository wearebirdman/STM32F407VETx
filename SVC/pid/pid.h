#ifndef __PID_H
#define __PID_H

#include "main.h"

/* 增量式PID
 * 输出为控制量增量Δu，适用于舵机角度、步进电机位置等积分型执行器 */
typedef struct
{
    float kp;             /* 比例增益 */
    float ki;             /* 积分增益 */
    float kd;             /* 微分增益 */
    float ek;             /* 当前误差 e(k) */
    float ek1;            /* 前次误差 e(k-1) */
    float ek2;            /* 前前次误差 e(k-2) */
    float output_max;     /* 输出增量限幅（绝对值，对称钳位） */
    float integral_max;   /* 误差累积限幅，防积分饱和 */
    float deadband;       /* 死区阈值（误差绝对值小于此值不调节） */
} PidInc_t;

/* 位置式PID
 * 输出为控制量绝对值u，适用于直流电机调速、加热器占空比等 */
typedef struct
{
    float kp;             /* 比例增益 */
    float ki;             /* 积分增益 */
    float kd;             /* 微分增益 */
    float ek;             /* 当前误差 */
    float ek1;            /* 前次误差（用于微分计算） */
    float integral;       /* 积分累加项 */
    float output_max;     /* 输出限幅（绝对值，对称钳位） */
    float integral_max;   /* 积分累加限幅，防积分饱和 */
    float deadband;       /* 死区阈值（误差绝对值小于此值不调节） */
} PidPos_t;

/* 初始化（使用前必须调用） */
void PidInc_Init(PidInc_t *pid);
void PidPos_Init(PidPos_t *pid);

/* PID计算
 * pid: 指向已初始化的PID结构体
 * target: 设定值/目标值
 * actual: 实际值/反馈值
 * 增量式返回增量Δu；位置式返回绝对值u */
float PidInc_Calc(PidInc_t *pid, float target, float actual);
float PidPos_Calc(PidPos_t *pid, float target, float actual);

/* 复位（清除误差历史和积分项，不改变kp/ki/kd/限幅参数） */
void PidInc_Reset(PidInc_t *pid);
void PidPos_Reset(PidPos_t *pid);

#endif /* __PID_H */
