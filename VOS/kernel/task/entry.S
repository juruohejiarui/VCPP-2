#include "../includes/linkage.h"

ENTRY(Task_kernelThreadEntry)
    movq %rsp, %rdi
    call checkTaskStack
    jmp restoreAll

ENTRY(Syscall_entry)
    // switch to kernel stack
    movq $0x100000, %rbx
    movq 0x18(%rbx), %rbx
    movq %rsp, 0x10(%rbx)
    movq 0x18(%rbx), %rsp
    movq %rsp, 0x08(%rbx)
    // save the registers
    subq $0x38, %rsp
    pushq %rax
    movq %es, %rbx
    pushq %rbx
    movq %ds, %rbx
    pushq %rbx
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

    leaq Syscall_exit(%rip), %rax
    pushq %rax
    jmp Syscall_handler

ENTRY(Syscall_exit)
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
    sysretq
