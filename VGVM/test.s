    .section .data

    .section .bss
    .section .text
    .global _start

_start:
    endbr64
	mov %rax, -0x08(%rax)
	mov %rax, 0x08(%rax)
    .fill 512, 1, 1
    jc _start
