#ifndef __FILTER_H
#define __FILTER_H

#include "main.h"

/* 一阶低通滤波（一阶IIR）
 * y(k) = y(k-1) + alpha * [x(k) - y(k-1)]
 * alpha越小，滤波越强，但滞后越大；alpha=1时无滤波
 * 适用于:温度、电压、IMU等缓变信号去噪 */
typedef struct
{
    float alpha;     /* 滤波系数（0~1） */
    float y_prev;    /* 上一次输出 */
    uint8_t init;    /* 是否已初始化（首次输入直接作为初值） */
} FilterLpf_t;

/* 滑动平均滤波
 * 维护长度为window_size的环形缓冲区，每次取平均值
 * 适用于:去除随机噪声，但窗口越大滞后越大 */
#define FILTER_AVG_WINDOW_MAX  16   /* 滑动窗口最大长度 */

typedef struct
{
    float buf[FILTER_AVG_WINDOW_MAX];   /* 环形缓冲区 */
    uint8_t size;                        /* 窗口长度（<= FILTER_AVG_WINDOW_MAX） */
    uint8_t index;                       /* 当前写入位置 */
    uint8_t count;                       /* 已写入数据个数 */
    float sum;                           /* 窗口内数据求和（增量维护） */
} FilterAvg_t;

/* 中值滤波
 * 维护长度为window_size的环形缓冲区，每次取中值
 * 对脉冲干扰有很强抑制，但计算量略大（需要排序）
 * window_size建议为奇数:3/5/7 */
#define FILTER_MED_WINDOW_MAX  9    /* 中值窗口最大长度 */

typedef struct
{
    float buf[FILTER_MED_WINDOW_MAX];   /* 环形缓冲区 */
    uint8_t size;                        /* 窗口长度（建议奇数） */
    uint8_t index;                       /* 当前写入位置 */
    uint8_t count;                       /* 已写入数据个数 */
} FilterMed_t;

/* 一阶低通 */
void  FilterLpf_Init(FilterLpf_t *f, float alpha);
float FilterLpf_Calc(FilterLpf_t *f, float x);

/* 滑动平均 */
void  FilterAvg_Init(FilterAvg_t *f, uint8_t window_size);
float FilterAvg_Calc(FilterAvg_t *f, float x);

/* 中值滤波 */
void  FilterMed_Init(FilterMed_t *f, uint8_t window_size);
float FilterMed_Calc(FilterMed_t *f, float x);

#endif /* __FILTER_H */
