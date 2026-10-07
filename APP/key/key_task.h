#ifndef __KEY_TASK_H
#define __KEY_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* 按键消息队列句柄（freertos.c 中创建） */
extern osMessageQueueId_t q_KeyMsgHandle;

void Key_Proc(void *argument);

#endif /* __KEY_TASK_H */
