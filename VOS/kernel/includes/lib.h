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

#ifndef __LIB_H__
#define __LIB_H__

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;


#define NULL 0

#define container_of(ptr,type,member)									\
({																		\
	typeof(((type *)0)->member) * p = (ptr);							\
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));	\
})


#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")

typedef struct tmpList
{
	struct tmpList * prev;
	struct tmpList * next;
} List;

void List_init(List *);
void List_addBehind(List *prevEle, List *newEle);
void List_addBefore(List *nxtEle, List *newEle);
void List_del(List *);
long List_isEmpty(List *);
List * List_prev(List *);
List * List_next(List *);

void * memcpy(void * src, void *dst, long size);
int memcmp(void *val1, void *val2, long size);
void * memset(void * dst, unsigned char val, long size);

char * strcpy(char *, char *);
char * strncpy(char *, char * Src, long);
char * strcat(char *, char *);
int strcmp(char *, char *);
int strncmp(char *, char *, long);
int strlen(char *);

u64 BIT_set(u64 *addr, u64 pos);
u64 BIT_get(u64 *addr, u64 pos);
u64 BIT_clear(u64 *addr, u64 pos);

u8 IO_in8(u16 port);
u32 IO_in32(u16 port);
void IO_out8(u16 port, u8 data);
void IO_out32(u16 port, u32 data);

unsigned long rdmsr(unsigned long);
void wrmsr(unsigned long, unsigned long);

#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

#endif
