#ifndef __INTERP_H
#define __INTERP_H

#include "main.h"

/* 线性插值
 * 给定起点a、终点b、比例t（0~1），返回a到b之间的线性插值 */
float Interp_Linear(float a, float b, float t);

/* 平滑步进插值
 * 给定当前值current、目标值target、最大步长max_step，
 * 返回向target逼近一步后的值（不超过max_step）
 * 适用于舵机平滑运动:每周期调用一次，逐步逼近目标 */
float Interp_Step(float current, float target, float max_step);

/* 多轴平滑步进
 * 对count个轴同时做平滑步进插值
 * current[]: 当前各轴位置
 * target[]: 目标各轴位置
 * out[]: 输出各轴插值后位置
 * count: 轴数
 * max_step: 单步最大变化量（度）
 * 返回:是否已全部到达目标（1=到达, 0=未到达） */
uint8_t Interp_StepMulti(float *current, const float *target,
                         float *out, uint8_t count, float max_step);

#endif /* __INTERP_H */
