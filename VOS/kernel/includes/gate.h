#ifndef __GATE_H__
#define __GATE_H__

struct DescStruct {
    unsigned char data[8];
};
struct GateStruct {
    unsigned char data[16];
};
extern struct DescStruct GDT_Table[];
extern struct GateStruct IDT_Table[];
extern unsigned int TSS64_Table[26];

#define setGate(gateSelAddr, attr, ist, codeAddr) \
    do { \
        unsigned long ___d0, ___d1; \
        __asm__ __volatile__ ( \
            "movw %%dx, %%ax \n\t" \
            "andq $0x7, %%rcx   \n\t"\
            "addq %4, %%rcx \n\t" \
            "shlq $32, %%rcx \n\t" \
            "addq %%rcx, %%rax \n\t" \
            "xorq %%rcx, %%rcx \n\t" \
            "movl %%edx, %%ecx \n\t" \
            "shrq $16, %%rcx \n\t" \
            "shlq $48, %%rcx \n\t" \
            "addq %%rcx, %%rax \n\t" \
            "movq %%rax, %0 \n\t" \
            "shrq $32, %%rdx \n\t" \
            "movq %%rdx, %1 \n\t" \
            : "=m"(*((unsigned long *)(gateSelAddr))), \
              "=m"(*(((unsigned long *)(gateSelAddr)) + 1)), \
              "=&a"(___d0), \
              "=&d"(___d1) \
            : "i"(attr << 8), \
              "3"((unsigned long *)(codeAddr)), \
              "2"(0x8 << 16), \
              "c"(ist) \
            : "memory" \
        ); \
    } while(0)
#define loadTR(n) 							\
do{									\
	__asm__ __volatile__(	"ltr	%%ax"				\
				:					\
				:"a"(n << 3)				\
				:"memory");				\
}while(0)

inline void setIntrGate(unsigned int n, unsigned char ist, void *addr) {
    setGate((unsigned long *)(&IDT_Table[n]), 0x8E, ist, addr);
}
inline void setTrapGate(unsigned int n, unsigned char ist, void *addr) {
    setGate((unsigned long *)(&IDT_Table[n]), 0x8F, ist, addr);
}
inline void setSystemGate(unsigned int n, unsigned char ist, void *addr) {
    setGate((unsigned long *)(&IDT_Table[n]), 0xEF, ist, addr);
}

void setTSS64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7) {
	*(unsigned long *)(TSS64_Table+1) = rsp0;
	*(unsigned long *)(TSS64_Table+3) = rsp1;
	*(unsigned long *)(TSS64_Table+5) = rsp2;

	*(unsigned long *)(TSS64_Table+9) = ist1;
	*(unsigned long *)(TSS64_Table+11) = ist2;
	*(unsigned long *)(TSS64_Table+13) = ist3;
	*(unsigned long *)(TSS64_Table+15) = ist4;
	*(unsigned long *)(TSS64_Table+17) = ist5;
	*(unsigned long *)(TSS64_Table+19) = ist6;
	*(unsigned long *)(TSS64_Table+21) = ist7;	
}


#endif