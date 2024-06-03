#include "../includes/linkage.h"

TaskStruct_state	= 0x10
TaskStruct_thread	= 0x18
TaskStruct_mem		= 0x20
TaskStruct_tss		= 0x28
TaskStruct_vRunTime	= 0x40

ThreadStruct_rip	= 0x00
ThreadStruct_rsp0	= 0x08
ThreadStruct_rsp3	= 0x10
ThreadStruct_rsp	= 0x18

TaskMemStruct_pgdPhyAddr = 0x08
/* Task_switch(TaskStruct *next) */
ENTRY(Task_switch)
	pushq %rbp
	pushq %rax
	movq $0xffffffffff800000, %rcx
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
	movq %rax, %cr3
	mfence
	movq $0xffffffffff800000, %rcx
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
	