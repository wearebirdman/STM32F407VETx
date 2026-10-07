#include "filter.h"

/* ===================== 一阶低通滤波 ===================== */

void FilterLpf_Init(FilterLpf_t *f, float alpha)
{
    if (f == NULL)
        return;
    f->alpha  = alpha;
    f->y_prev = 0.0f;
    f->init   = 0;
}

float FilterLpf_Calc(FilterLpf_t *f, float x)
{
    if (f == NULL)
        return x;

    /* 首次输入直接作为初值，避免从0开始爬坡 */
    if (!f->init)
    {
        f->y_prev = x;
        f->init   = 1;
        return x;
    }

    float y = f->y_prev + f->alpha * (x - f->y_prev);
    f->y_prev = y;
    return y;
}

/* ===================== 滑动平均滤波 ===================== */

void FilterAvg_Init(FilterAvg_t *f, uint8_t window_size)
{
    if (f == NULL)
        return;
    if (window_size == 0)
        window_size = 1;
    if (window_size > FILTER_AVG_WINDOW_MAX)
        window_size = FILTER_AVG_WINDOW_MAX;

    f->size  = window_size;
    f->index = 0;
    f->count = 0;
    f->sum   = 0.0f;
    for (uint8_t i = 0; i < FILTER_AVG_WINDOW_MAX; i++)
        f->buf[i] = 0.0f;
}

float FilterAvg_Calc(FilterAvg_t *f, float x)
{
    if (f == NULL)
        return x;

    if (f->count < f->size)
    {
        /* 窗口未满:直接累加 */
        f->buf[f->index] = x;
        f->sum += x;
        f->count++;
        f->index = (f->index + 1) % f->size;
        return f->sum / f->count;
    }
    else
    {
        /* 窗口已满:减去最旧值，加入新值 */
        f->sum -= f->buf[f->index];
        f->buf[f->index] = x;
        f->sum += x;
        f->index = (f->index + 1) % f->size;
        return f->sum / f->size;
    }
}

/* ===================== 中值滤波 ===================== */

void FilterMed_Init(FilterMed_t *f, uint8_t window_size)
{
    if (f == NULL)
        return;
    if (window_size == 0)
        window_size = 1;
    if (window_size > FILTER_MED_WINDOW_MAX)
        window_size = FILTER_MED_WINDOW_MAX;

    f->size  = window_size;
    f->index = 0;
    f->count = 0;
    for (uint8_t i = 0; i < FILTER_MED_WINDOW_MAX; i++)
        f->buf[i] = 0.0f;
}

float FilterMed_Calc(FilterMed_t *f, float x)
{
    if (f == NULL)
        return x;

    /* 写入新值 */
    f->buf[f->index] = x;
    f->index = (f->index + 1) % f->size;
    if (f->count < f->size)
        f->count++;

    /* 拷贝到临时数组并排序（冒泡排序，窗口小，够用） */
    float tmp[FILTER_MED_WINDOW_MAX];
    for (uint8_t i = 0; i < f->count; i++)
        tmp[i] = f->buf[i];

    for (uint8_t i = 0; i < f->count - 1; i++)
    {
        for (uint8_t j = 0; j < f->count - 1 - i; j++)
        {
            if (tmp[j] > tmp[j + 1])
            {
                float t = tmp[j];
                tmp[j] = tmp[j + 1];
                tmp[j + 1] = t;
            }
        }
    }

    return tmp[f->count / 2];
}
