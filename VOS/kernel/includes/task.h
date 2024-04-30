#ifndef __TASK_H__
#define __TASK_H__

#include "../task/desc.h"

TaskStruct *Task_getCurrent() {
    TaskStruct *current;
    __asm__ __volatile__("andq %%rsp, %0" : "=r"(current) : "0"(~32768ul));
    return current;
}

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
}
#endif