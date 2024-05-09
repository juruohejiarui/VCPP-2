#include "../includes/linkage.h"

funcOffset      = 0x88
errCodeOffset   = 0x90

ENTRY(Gate_loadTR)
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
ENTRY(Intr_retFromIntr)
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
    movq %rdi, %ss
    movq %rdi, %cs
    movq %rsp, %rdi

    call *%rdx
    jmp retFromException

ENTRY(debug)
    pushq $0
    pushq %rax
    leaq doDebug(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(nmi)
    /* no error code since it is not an exception */
    pushq %rax
    cld
    pushq %rax
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

    movq $0x10, %rdi
    movq %rdi, %ds
    movq %rdi, %es
    movq $0, %rsi
    movq %rsp, %rdi

    callq doNMI
    jmp restoreAll

ENTRY(int3)
    pushq $0
    pushq %rax
    leaq doInt3(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(overflow)
    pushq $0
    pushq %rax
    leaq doOverflow(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(bounds)
    pushq $0
    pushq %rax
    leaq doBounds(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(undefinedOpcode)
    pushq	$0
	pushq	%rax
    leaq doUndefinedOpcode(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode
    
ENTRY(devNotAvailable)
    pushq $0
    pushq %rax
    leaq doDevNotAvailable(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(doubleFault)
    pushq %rax
    leaq doDoubleFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(coprocessorSegmentOverrun)
    pushq $0
    pushq %rax
    leaq doCoprocessorSegmentOverrun(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(invalidTSS)
    pushq %rax
    leaq doInvalidTSS(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(segmentNotPresent)
    pushq %rax
    leaq doSegmentNotPresent(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(stackSegmentFault)
    pushq %rax
    leaq doStackSegmentFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(generalProtection)
    pushq %rax
    leaq doGeneralProtection(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(pageFault)
    pushq %rax
    leaq doPageFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(x87FPUError)
    pushq $0
    pushq %rax
    leaq doX87FPUError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(alignmentCheck)
    pushq %rax
    leaq doAlignmentCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(machineCheck)
    pushq $0
    pushq %rax
    leaq doMachineCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(simdError)
    pushq $0
    pushq %rax
    leaq doSIMDError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(virtualizationError)
    pushq $0
    pushq %rax
    leaq doVirtualizationError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode