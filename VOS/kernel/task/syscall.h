#ifndef __TASK_SYSCALL_H__
#define __TASK_SYSCALL_H__

#include "../includes/lib.h"
#include <stdarg.h>

/// @brief The interface for user level program to access syscall
/// @param id the index of the syscall
/// @param others the params of the system call
/// @return the result of the syscall
u64 Syscall_usrAPI(u64 id, ...);

void Task_switchToUsr(u64 (*entry)(), u64 rspUser, u64 arg);
u64 Task_initUsrLevel(u64 arg);

void Init_syscall();

#endif