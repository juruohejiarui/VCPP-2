#include "../includes/linkage.h"

funcOffset      = 0x88
errCodeOffset   = 0x90

ENTRY(loadTR)
    movw %di, %ax
    shlw $3, %ax
    ltr %ax
    xorw %ax, %ax
    retq

restoreAll:
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
    addq $0x10, %rsp
    iretq

retFromException:
ENTRY(retFromIntr)
    jmp restoreAll

ENTRY(divideError)
    pushq $0
    pushq %rax
    leaq doDivideError(%rip), %rax
    xchgq %rax, (%rsp)

errorCode:
    pushq %rax
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
    pushq %rax
    xorq %rax, %rax
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

    cld 
    movq errCodeOffset(%rsp), %rsi
    movq funcOffset(%rsp), %rdx
    movq $0x10, %rdi
    movq %rdi, %ds
    movq %rdi, %es
    movq %rsp, %rdi

    call *%rdx
    jmp retFromException

