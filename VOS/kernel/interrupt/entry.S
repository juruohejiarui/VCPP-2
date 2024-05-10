#include "../includes/linkage.h"


offsetR15		= 0x00
offsetR14		= 0x08
offsetR13		= 0x10
offsetR12		= 0x18
offsetR11		= 0x20
offsetR10		= 0x28
offsetR9		= 0x30
offsetR8		= 0x38
offsetRbx		= 0x40
offsetRcx		= 0x48
offsetRdx		= 0x50
offsetRsi		= 0x58
offsetRdi		= 0x60
offsetRbp		= 0x68
offsetDs		= 0x70
offsetEs		= 0x78
offsetRax		= 0x80
offsetFunc		= 0x88
offsetErrorCode	= 0x90
offsetRip		= 0x98
offsetCs		= 0xa0
offsetRflags	= 0xa8
offsetRsp		= 0xb0
offsetSs		= 0xb8



ENTRY(Gate_loadTR)
    movw %di, %ax
    shlw $3, %ax
    ltr %ax
    xorw %ax, %ax
    retq

ENTRY(restoreAll)
	// movq %rsp, %rdi
	// call Task_checkPtRegInStack
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
    test %rax, %rax
    jz restoreAll
    jmp Task_switch

ENTRY(divideError)
    cld 
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

    movq offsetErrorCode(%rsp), %rsi
    movq offsetFunc(%rsp), %rdx
    movq $0x10, %rdi
    movq %rdi, %ds
    movq %rdi, %es
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
    cld
    /* no error code since it is not an exception */
    pushq %rax
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
    cld 
    pushq $0
    pushq %rax
    leaq doInt3(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(overflow)
    cld 
    pushq $0
    pushq %rax
    leaq doOverflow(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(bounds)
    cld 
    pushq $0
    pushq %rax
    leaq doBounds(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(undefinedOpcode)
    cld 
    pushq	$0
	pushq	%rax
    leaq doUndefinedOpcode(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode
    
ENTRY(devNotAvailable)
    cld 
    pushq $0
    pushq %rax
    leaq doDevNotAvailable(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(doubleFault)
    cld 
    pushq %rax
    leaq doDoubleFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(coprocessorSegmentOverrun)
    cld 
    pushq $0
    pushq %rax
    leaq doCoprocessorSegmentOverrun(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(invalidTSS)
    cld 
    pushq %rax
    leaq doInvalidTSS(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(segmentNotPresent)
    cld 
    pushq %rax
    leaq doSegmentNotPresent(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(stackSegmentFault)
    cld 
    pushq %rax
    leaq doStackSegmentFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(generalProtection)
    cld 
    pushq %rax
    leaq doGeneralProtection(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(pageFault)
	cld
    pushq %rax
    leaq doPageFault(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(x87FPUError)
    cld 
    pushq $0
    pushq %rax
    leaq doX87FPUError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(alignmentCheck)
    cld 
    pushq %rax
    leaq doAlignmentCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(machineCheck)
    cld 
    pushq $0
    pushq %rax
    leaq doMachineCheck(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(simdError)
    cld 
    pushq $0
    pushq %rax
    leaq doSIMDError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode

ENTRY(virtualizationError)
    cld 
    pushq $0
    pushq %rax
    leaq doVirtualizationError(%rip), %rax
    xchgq %rax, (%rsp)
    jmp errorCode