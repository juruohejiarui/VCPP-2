/***************************************************
*		版权声明
*
*	本操作系统名为：MINE
*	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
*	只允许个人学习以及公开交流使用
*
*	代码最终所有权及解释权归田宇所有；
*
*	本模块作者：	田宇
*	EMail:		345538255@qq.com
*
*
***************************************************/

#ifndef __GATE_H__
#define __GATE_H__

struct desc_struct
{
	unsigned char x[8];
};
typedef struct desc_struct desc_s;

struct gate_struct
{
	unsigned char x[16];
};
typedef struct gate_struct gate_s;

extern desc_s GDT_Table[];
extern gate_s IDT_Table[];
extern unsigned int TSS64_Table[26];

/*

*/

#define _set_gate(gate_selector_addr, attr, ist, code_addr)                                                 \
	do                                                                                                      \
	{                                                                                                       \
		unsigned long __d0, __d1;                                                                           \
		__asm__ __volatile__("movw	%%dx,	%%ax	\n\t"                                                   \
							 "andq	$0x7,	%%rcx	\n\t"                                                   \
							 "addq	%4,	%%rcx	\n\t"                                                       \
							 "shlq	$32,	%%rcx	\n\t"                                                   \
							 "addq	%%rcx,	%%rax	\n\t"                                                   \
							 "xorq	%%rcx,	%%rcx	\n\t"                                                   \
							 "movl	%%edx,	%%ecx	\n\t"                                                   \
							 "shrq	$16,	%%rcx	\n\t"                                                   \
							 "shlq	$48,	%%rcx	\n\t"                                                   \
							 "addq	%%rcx,	%%rax	\n\t"                                                   \
							 "movq	%%rax,	%0	\n\t"                                                       \
							 "shrq	$32,	%%rdx	\n\t"                                                   \
							 "movq	%%rdx,	%1	\n\t"                                                       \
							 : "=m"(*((unsigned long *)(gate_selector_addr))),                              \
							   "=m"(*(1 + (unsigned long *)(gate_selector_addr))), "=&a"(__d0), "=&d"(__d1) \
							 : "i"(attr << 8),                                                              \
							   "3"((unsigned long *)(code_addr)), "2"(0x8 << 16), "c"(ist)                  \
							 : "memory");                                                                   \
	} while (0)

#define loadTR(n)                         \
	do                                     \
	{                                      \
		__asm__ __volatile__("ltr	%%ax"  \
							 :             \
							 : "a"(n << 3) \
							 : "memory");  \
	} while (0)

void setIntrGate(unsigned int, unsigned char, void *);
void setTrapGate(unsigned int, unsigned char, void *);
void setSystemGate(unsigned int, unsigned char, void *);
void setSystemIntrGate(unsigned int, unsigned char, void *);
void setTSS64(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long,
			   unsigned long, unsigned long, unsigned long, unsigned long);

#endif
