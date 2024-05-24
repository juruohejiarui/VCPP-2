#ifndef __TASK_SYSCALL_H__
#define __TASK_SYSCALL_H__

#include "../includes/lib.h"
#include <stdarg.h>

#define Syscall_num 1024

/// @brief The interface for user level program to access syscall
/// @param id the index of the syscall
/// @param others the params of the system call
/// @return the result of the syscall
u64 Task_Syscall_usrAPI(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

void Task_switchToUsr(u64 (*entry)(u64), u64 arg);

void Task_Syscall_init();

#endif