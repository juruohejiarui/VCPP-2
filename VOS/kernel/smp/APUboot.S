#include "../includes/linkage.h"

.balign	0x1000

.text
.code16

ENTRY(SMP_APUBootStart)

SMP_APUBootBase = .
	cli
	wbinvd

	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	movl $(SMP_APUBootTmpStkEnd - SMP_APUBootBase), %esp

	mov 	%cs, %ax
	movzx 	%ax, %esi
	shll 	$4, %esi

	leal (SMP_APUCode32 - SMP_APUBootBase)(%esi), %eax
	movl %eax, SMP_APUCode32Vector - SMP_APUBootBase
	
	leal (SMP_APUCode64 - SMP_APUBootBase)(%esi), %eax
	movl %eax, SMP_APUCode64Vector - SMP_APUBootBase

	leal (SMP_APUTmpGDT - SMP_APUBootBase)(%esi), %eax
	movl %eax, (SMP_APUTmpGDT + 2 - SMP_APUBootBase)

	lidtl SMP_APUTmpIDT - SMP_APUBootBase
	lgdtl SMP_APUTmpGDT - SMP_APUBootBase

	smsw %ax
	bts $0, %ax
	lmsw %ax

	ljmpl *(SMP_APUCode32Vector - SMP_APUBootBase)

.code32
.balign 4

SMP_APUCode32:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	leal (SMP_APUBootTmpStkEnd - SMP_APUBootBase)(%esi), %eax
	movl %eax, %esp

	// enable PAE
	movl %cr4, %eax
	bts $5, %eax
	movl %eax, %cr4

	// set page table
	movl $0x101000, %eax
	movl %eax, %cr3


	// enable long mode
	movl $0xC0000080, %ecx
	rdmsr

	bts $8, %eax
	wrmsr

	// enable PE and paging
	movl %cr0, %eax
	bts $0, %eax
	bts $31, %eax
	movl %eax, %cr0

	ljmp *(SMP_APUCode64Vector - SMP_APUBootBase)(%esi)

.code64
.balign 4
SMP_APUCode64:
	movq $0x20, %rax
	movq %rax, %ds
	movq %rax, %es
	movq %rax, %fs
	movq %rax, %gs
	movq %rax, %ss

	// jmp to head.S
	movq $0x100000, %rax
	jmpq *%rax

	hlt

.balign 4
SMP_APUTmpIDT:
	.word 0
	.word 0, 0

.balign 4
SMP_APUTmpGDT:
	.short SMP_APUTmpGDTEnd - SMP_APUTmpGDT - 1
	.long SMP_APUTmpGDT - SMP_APUBootBase
	.short 0
	.quad 0x00cf9a000000ffff
	.quad 0x00cf92000000ffff
	.quad 0x0020980000000000
	.quad 0x0000920000000000
SMP_APUTmpGDTEnd:

.balign 4
SMP_APUCode32Vector:
	.long SMP_APUCode32 - SMP_APUBootBase
	.word 0x08, 0

.balign 4
SMP_APUCode64Vector:
	.long SMP_APUCode64 - SMP_APUBootBase
	.word 0x18, 0

.balign 4
SMP_APUBootTmpStkStart:
	.org 0x400
SMP_APUBootTmpStkEnd:

ENTRY(SMP_APUBootEnd)
	