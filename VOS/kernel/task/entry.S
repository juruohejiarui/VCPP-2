#include "../includes/linkage.h"

ENTRY(Task_kernelThreadEntry)
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
    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es
    popq %rax
    addq $0x38, %rsp
    movq %rdx, %rdi
    call *%rbx // just to the function of the thread
    movq %rax, %rdi // get the return value of the thread and store it to the first param
    call Task_doExit

ENTRY(Syscall_entry)
    // save the registers
    subq $0x38, %rsp
    pushq $0
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    call Syscall_handler
    jmp Syscall_exit

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
    // skip the zero
    addq $8, %rsp
    addq $0x38, %rsp
    sysretq