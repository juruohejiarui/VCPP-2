#include "../includes/linkage.h"

ENTRY(Task_kernelThreadEntry)
    jmp restoreAll

ENTRY(Syscall_entry)
    // switch to kernel stack
    movq $0xffffffffff800000, %rbx
    movq 0x18(%rbx), %rbx
    movq %rsp, 0x10(%rbx)
    movq 0x8(%rbx), %rsp
    movq %rsp, 0x18(%rbx)
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
    leaq Syscall_exit(%rip), %rax
    pushq %rax
    jmp Syscall_handler

ENTRY(Syscall_exit)
    popq %r11
    popq %rcx
    popq %rbx
    movq %rbx, %ds
    popq %rbx
    movq %rbx, %es
    movq $0xffffffffff800000, %rbx
    movq 0x18(%rbx), %rbx
    movq %rsp, 0x8(%rbx)
    movq 0x10(%rbx), %rsp
    movq %rsp, 0x18(%rbx)
    sysretq
