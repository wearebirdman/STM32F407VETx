#ifndef __LED_TASK_H
#define __LED_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* LED 请求队列句柄（freertos.c 中创建） */
extern osMessageQueueId_t q_LedReqHandle;

void Led_Disp(void *argument);

#endif /* __LED_TASK_H */
