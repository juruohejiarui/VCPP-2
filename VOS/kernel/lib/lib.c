#include "../includes/lib.h"

inline void List_init(List *list)
{
	list->prev = list;
	list->next = list;
}

inline void List_addBehind(List *entry, List *new) ////add to entry behind
{
	new->next = entry->next;
	new->prev = entry;
	new->next->prev = new;
	entry->next = new;
}

inline void List_addBefore(List *entry, List *new) ////add to entry behind
{
	new->next = entry;
	entry->prev->next = new;
	new->prev = entry->prev;
	entry->prev = new;
}

inline void List_del(List *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

inline long List_isEmpty(List *entry) {
	return entry == entry->next && entry->prev == entry;
}

inline List *List_prev(List *entry)
{
	return entry->prev;
}

inline List *List_next(List *entry)
{
	return entry->next;
}

/*
		From => To memory copy Num bytes
*/

inline void *memcpy(void *From, void *To, long Num)
{
	int d0, d1, d2;
	__asm__ __volatile__("cld				\n\t"
						 "rep				\n\t"
						 "movsq				\n\t"
						 "testb	$4, %b4		\n\t"
						 "je	1f			\n\t"
						 "movsl				\n\t"
						 "1:\ttestb	$2, %b4	\n\t"
						 "je	2f			\n\t"
						 "movsw				\n\t"
						 "2:\ttestb	$1,%b4	\n\t"
						 "je	3f			\n\t"
						 "movsb				\n\t"
						 "3:				\n\t"
						 : "=&c"(d0), "=&D"(d1), "=&S"(d2)
						 : "0"(Num / 8), "q"(Num), "1"(To), "2"(From)
						 : "memory");
	return To;
}

/*
		FirstPart = SecondPart		=>	 0
		FirstPart > SecondPart		=>	 1
		FirstPart < SecondPart		=>	-1
*/

inline int memcmp(void *FirstPart, void *SecondPart, long Count)
{
	register int __res;

	__asm__ __volatile__("cld				\n\t" //clean direct
						 "repe				\n\t" //repeat if equal
						 "cmpsb				\n\t"
						 "je	1f			\n\t"
						 "movl	$1,	%%eax	\n\t"
						 "jl	1f			\n\t"
						 "negl	%%eax		\n\t"
						 "1:				\n\t"
						 : "=a"(__res)
						 : "0"(0), "D"(FirstPart), "S"(SecondPart), "c"(Count)
						 :);
	return __res;
}

inline void *memset(void *Address, unsigned char C, long Count)
{
	int d0, d1;
	unsigned long tmp = C * 0x0101010101010101UL;
	__asm__ __volatile__("cld				\n\t"
						 "rep				\n\t"
						 "stosq				\n\t"
						 "testb	$4, %b3		\n\t"
						 "je	1f			\n\t"
						 "stosl				\n\t"
						 "1:\ttestb	$2, %b3	\n\t"
						 "je	2f			\n\t"
						 "stosw				\n\t"
						 "2:\ttestb	$1, %b3	\n\t"
						 "je	3f			\n\t"
						 "stosb				\n\t"
						 "3:				\n\t"
						 : "=&c"(d0), "=&D"(d1)
						 : "a"(tmp), "q"(Count), "0"(Count / 8), "1"(Address)
						 : "memory");
	return Address;
}

/*
		string copy
*/

inline char *strcpy(char *Dest, char *Src)
{
	__asm__ __volatile__("cld				\n\t"
						 "1:				\n\t"
						 "lodsb				\n\t"
						 "stosb				\n\t"
						 "testb	%%al, %%al	\n\t"
						 "jne	1b			\n\t"
						 :
						 : "S"(Src), "D"(Dest)
						 :

	);
	return Dest;
}

/*
		string copy number bytes
*/

inline char *strncpy(char *Dest, char *Src, long Count)
{
	__asm__ __volatile__("cld				\n\t"
						 "1:				\n\t"
						 "decq	%2			\n\t"
						 "js	2f			\n\t"
						 "lodsb				\n\t"
						 "stosb				\n\t"
						 "testb	%%al, %%al	\n\t"
						 "jne	1b			\n\t"
						 "rep				\n\t"
						 "stosb				\n\t"
						 "2:				\n\t"
						 :
						 : "S"(Src), "D"(Dest), "c"(Count)
						 :);
	return Dest;
}

/*
		string cat Dest + Src
*/

inline char *strcat(char *Dest, char *Src)
{
	__asm__ __volatile__("cld				\n\t"
						 "repne				\n\t"
						 "scasb				\n\t"
						 "decq	%1			\n\t"
						 "1:				\n\t"
						 "lodsb				\n\t"
						 "stosb				\n\r"
						 "testb	%%al, %%al	\n\t"
						 "jne	1b			\n\t"
						 :
						 : "S"(Src), "D"(Dest), "a"(0), "c"(0xffffffff)
						 :);
	return Dest;
}

/*
		string compare FirstPart and SecondPart
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strcmp(char *FirstPart, char *SecondPart)
{
	register int __res;
	__asm__ __volatile__("cld					\n\t"
						 "1:					\n\t"
						 "lodsb					\n\t"
						 "scasb					\n\t"
						 "jne	2f				\n\t"
						 "testb	%%al, %%al		\n\t"
						 "jne	1b				\n\t"
						 "xorl	%%eax, %%eax	\n\t"
						 "jmp	3f				\n\t"
						 "2:					\n\t"
						 "movl	$1,	%%eax		\n\t"
						 "jl	3f				\n\t"
						 "negl	%%eax			\n\t"
						 "3:					\n\t"
						 : "=a"(__res)
						 : "D"(FirstPart), "S"(SecondPart)
						 :);
	return __res;
}

/*
		string compare FirstPart and SecondPart with Count Bytes
		FirstPart = SecondPart =>  0
		FirstPart > SecondPart =>  1
		FirstPart < SecondPart => -1
*/

inline int strncmp(char *FirstPart, char *SecondPart, long Count)
{
	register int __res;
	__asm__ __volatile__("cld					\n\t"
						 "1:					\n\t"
						 "decq	%3				\n\t"
						 "js	2f				\n\t"
						 "lodsb					\n\t"
						 "scasb					\n\t"
						 "jne	3f				\n\t"
						 "testb	%%al, %%al		\n\t"
						 "jne	1b				\n\t"
						 "2:					\n\t"
						 "xorl	%%eax, %%eax	\n\t"
						 "jmp	4f				\n\t"
						 "3:					\n\t"
						 "movl	$1,	%%eax		\n\t"
						 "jl	4f				\n\t"
						 "negl	%%eax			\n\t"
						 "4:					\n\t"
						 : "=a"(__res)
						 : "D"(FirstPart), "S"(SecondPart), "c"(Count)
						 :);
	return __res;
}

/*

*/

inline int strlen(char *String)
{
	register int __res;
	__asm__ __volatile__("cld		\n\t"
						 "repne		\n\t"
						 "scasb		\n\t"
						 "notl	%0	\n\t"
						 "decl	%0	\n\t"
						 : "=c"(__res)
						 : "D"(String), "a"(0), "0"(0xffffffff)
						 :);
	return __res;
}

/*

*/

inline u64 BIT_set(u64 *addr, u64 nr) {
	return *addr | (1UL << nr);
}

/*

*/

inline u64 BIT_get(u64 *addr,u64 nr) {
	return *addr & (1UL << nr);
}

/*

*/

inline u64 BIT_clear(u64 *addr, u64 nr) {
	return *addr & (~(1UL << nr));
}

/*

*/

inline u8 IO_in8(u16 port)
{
	unsigned char ret = 0;
	__asm__ __volatile__("inb	%%dx, %0	\n\t"
						 "mfence			\n\t"
						 : "=a"(ret)
						 : "d"(port)
						 : "memory");
	return ret;
}

/*

*/

inline u32 IO_in32(u16 port)
{
	unsigned int ret = 0;
	__asm__ __volatile__("inl	%%dx, %0	\n\t"
						 "mfence			\n\t"
						 : "=a"(ret)
						 : "d"(port)
						 : "memory");
	return ret;
}

/*

*/

inline void IO_out8(u16 port, u8 value)
{
	__asm__ __volatile__("outb	%0,	%%dx	\n\t"
						 "mfence			\n\t"
						 :
						 : "a"(value), "d"(port)
						 : "memory");
}

/*

*/

inline void IO_out32(u16 port, u32 value)
{
	__asm__ __volatile__("outl	%0,	%%dx	\n\t"
						 "mfence			\n\t"
						 :
						 : "a"(value), "d"(port)
						 : "memory");
}

inline unsigned long rdmsr(unsigned long address)
{
	unsigned int tmp0 = 0;
	unsigned int tmp1 = 0;
	__asm__ __volatile__("rdmsr	\n\t"
						 : "=d"(tmp0), "=a"(tmp1)
						 : "c"(address)
						 : "memory");
	return (unsigned long)tmp0 << 32 | tmp1;
}

inline void wrmsr(unsigned long address, unsigned long value)
{
	__asm__ __volatile__("wrmsr	\n\t"
						 :
						 :"d"(value >> 32), "a"(value & 0xffffffff), "c"(address)
						 : "memory");
}