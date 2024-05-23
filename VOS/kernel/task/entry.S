#include "../includes/linkage.h"

ENTRY(Task_kernelThreadEntry)
    jmp restoreAll

ENTRY(Syscall_entry)
    // switch to kernel stack
    movq $0x100000, %rbx
    movq 0x18(%rbx), %rbx
    movq %rsp, 0x10(%rbx)
    movq 0x8(%rbx), %rsp
    movq %rsp, 0x18(%rbx)
    // save the registers
    subq $0x38, %rsp
    pushq %rax
    pushq %rbx
    movq %es, %rbx
    xchgq %rbx, (%rsp)
    pushq %rbx
    movq %ds, %rbx
    xchgq %rbx, (%rsp)
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    movq %rax, %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
	movq $0x10, %rax
	movq %rax, %ds
	movq %rax, %es
    leaq Syscall_exit(%rip), %rax
    pushq %rax
    jmp Syscall_handler

ENTRY(Syscall_exit)
    cli
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rbx
    movq %rbx, %ds
    popq %rbx
    movq %rbx, %es
    addq $8, %rsp
    addq $0x38, %rsp
    movq $0x100000, %rbx
    movq 0x18(%rbx), %rbx
    movq %rsp, 0x8(%rbx)
    movq 0x10(%rbx), %rsp
    movq %rsp, 0x18(%rbx)
	sti
    sysretq
