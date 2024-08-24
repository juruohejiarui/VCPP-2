#include "../includes/linkage.h"

TaskStruct_state	= 0x10
TaskStruct_thread	= 0x18
TaskStruct_mem		= 0x20
TaskStruct_tss		= 0x28
TaskStruct_vRunTime	= 0x40

ThreadStruct_rip	= 0x00
ThreadStruct_rsp3	= 0x08
ThreadStruct_rsp	= 0x10

Task_kernelStackEnd = 0xfffffffffffffff0


TaskMemStruct_pgdPhyAddr = 0x08
/* Task_switch(TaskStruct *next) */
ENTRY(Task_switch)
	pushq %rbp
	pushq %rax
	movq $0xffffffffffff8000, %rcx
	movq TaskStruct_thread(%rcx), %rbx
	// save rip, rsp, and cr3
	movq %rsp, ThreadStruct_rsp(%rbx)
	leaq Task_switch_back(%rip), %rax
	movq %rax, ThreadStruct_rip(%rbx)
	movq %cr3, %rax
	movq TaskStruct_mem(%rcx), %rbx
	movq %rax, TaskMemStruct_pgdPhyAddr(%rbx)
	pushq %rcx
	pushq %rdi
	movq %rdi, %rsi
	movq %rcx, %rdi
	callq Task_switchTo_inner
	popq %rsi
	popq %rdi
	// set up cr3, rsp and then rip
	movq TaskStruct_mem(%rsi), %rbx
	movq TaskMemStruct_pgdPhyAddr(%rbx), %rax
	movq %cr3, %rbx
	cmpq %rbx, %rax
	je TaskMemStruct_switchCR3_end
	movq %rax, %cr3
	mfence
TaskMemStruct_switchCR3_end:
	movq $0xffffffffffff8000, %rcx
	movq $0x2, %rax
	movq %rax, TaskStruct_state(%rcx)
	movq TaskStruct_thread(%rcx), %rbx
	movq ThreadStruct_rsp(%rbx), %rsp
	movq ThreadStruct_rip(%rbx), %rax
	jmp *%rax
Task_switch_back:
	// restore rax and rbp
	popq %rax
	popq %rbp
	retq

ENTRY(Task_kernelThreadEntry)
    jmp restoreAll

ENTRY(Syscall_entry)
    // switch to kernel stack
    movq $0xffffffffffff8000, %rbx
    movq TaskStruct_thread(%rbx), %rbx
    movq %rsp, ThreadStruct_rsp3(%rbx)
    movq $0xfffffffffffffff0, %rsp
    movq %rsp, ThreadStruct_rsp(%rbx)
    // save the registers
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
    pushq %rcx
    pushq %r11
	movq $0x10, %rax
	movq %rax, %ds
	movq %rax, %es
    callq Syscall_handler
	jmp Syscall_exit

ENTRY(Syscall_exit)
    popq %r11
    popq %rcx
    popq %rbx
    movq %rbx, %ds
    popq %rbx
    movq %rbx, %es

	// switch back to user stack
    movq $0xffffffffffff8000, %rbx
    movq TaskStruct_thread(%rbx), %rbx
	movq ThreadStruct_rsp3(%rbx), %rsp
	movq %rsp, ThreadStruct_rsp(%rbx)
    sysretq

	