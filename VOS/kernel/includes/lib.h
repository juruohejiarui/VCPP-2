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

typedef struct List
{
	struct List * prev;
	struct List * next;
} list_t;

void list_init(list_t *);
void list_add_to_behind(list_t *, list_t *);
void list_add_to_before(list_t *, list_t *);
void list_del(list_t *);
long list_is_empty(list_t *);
list_t * list_prev(list_t *);
list_t * list_next(list_t *);

void * memcpy(void *, void *, long);
int memcmp(void *, void *, long);
void * memset(void *, unsigned char, long);

char * strcpy(char *, char *);
char * strncpy(char *, char * Src, long);
char * strcat(char *, char *);
int strcmp(char *, char *);
int strncmp(char *, char *, long);
int strlen(char *);

unsigned long bit_set(unsigned long *, unsigned long);
unsigned long bit_get(unsigned long *, unsigned long);
unsigned long bit_clean(unsigned long *, unsigned long);

unsigned char io_in8(unsigned short);
unsigned int io_in32(unsigned short);
void io_out8(unsigned short, unsigned char);
void io_out32(unsigned short, unsigned int);

unsigned long rdmsr(unsigned long);
void wrmsr(unsigned long, unsigned long);

#define port_insw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;insw;mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port,buffer,nr)	\
__asm__ __volatile__("cld;rep;outsw;mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")

#endif
