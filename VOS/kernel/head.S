#include "linkage.h"

.section .text

ENTRY(_start)
    mov %ss, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    mov $0x7E00, %esp

    // set cr3
    movq $0x101000, %rax
    movq %rax, %cr3

    lgdt gdtPointer(%rip)
    
    lidt idtPointer(%rip)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    movq stackStart(%rip), %rsp

    movq $0x101000, %rax
    movq %rax, %cr3
    movq switchSeg(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq

switchSeg:
    .quad entry64

entry64:
    movq $0x10, %rax
    movq %rax, %ds
    movq %rax, %es
    movq %rax, %gs
    movq %rax, %ss
    movq stackStart(%rip), %rsp

intMsg:
    .asciz "Unknown interrupt or fault at RIP\n"

ENTRY(startStart)
    .quad initTaskUnion + 32768

// initial page table
.align 8

.org 0x1000

pml4e:
    .fill 256, 8, 0
    .quad 0x102007 /*0xffff800000000000 ~ 0xffff807FFFFFFFFF*/
    .fill 255, 8, 0

.org 0x2000

pdpte:
    .quad 0x103007
    .fill 511, 8, 0

.org 0x3000
    // the 7-th bit is 1, so the pointer of the page table item is to the physical address
    .quad	0x000087	
	.quad	0x200087
	.quad	0x400087
	.quad	0x600087
	.quad	0x800087
	.quad	0xa00087		/*0x a00000*/
	.quad	0xc00087
	.quad	0xe00087
	.quad	0x1000087
	.quad	0x1200087		/*0x1000000*/
	.quad	0x1400087
	.quad	0x1600087
	.quad	0x1800087
	.quad 	0x1a00087
	.quad 	0x1c00087
	.quad 	0x1e00087
	.quad 	0x2000087
	.quad 	0x2200087
	.quad 	0x2400087
	.quad 	0x2600087
	.quad 	0x2800087
	.quad 	0x2a00087
	.quad 	0x2c00087
	.quad 	0x2e00087
	.fill	488,8,0

.section .data

ENTRY(gdtTable)
    .quad 0x0000000000000000    /* NULL */
    .quad 0x0020980000000000    /* kernel code 64-bit */
    .quad 0x0000920000000000    /* kernel data 64-bit */
    .quad 0x0000000000000000 
    .quad 0x0000000000000000
    .quad 0x0020f80000000000    /* user code 64-bit */
    .quad 0x0000f20000000000    /* user data 64-bit */
    .quad 0x00cf9a000000ffff    /* kernel code 32-bit */
    .quad 0x00cf92000000ffff    /* kernel data 32-bit */
    .fill 10, 8, 0				/* TSS (jmp one segment <9>) in long-mode 128-bit 50*/
gdtEnd:

gdtPointer:
    gdtLimit:   .word gdtEnd - gdtTable - 1
    gdtBase:    .quad gdtTable

ENTRY(idtTable)
    .fill 512, 8, 0
idtEnd:

idtPointer:
    idtLimit:   .word idtEnd - idtTable - 1
    idtBase:    .quad idtTable

ENTRY(tss64Table)
    .fill 13, 8, 0
tss64End:

tss64Pointer:
    tss64Limit: .word tss64End - tss64Table - 1
    tss64Base:  .quad tss64Table