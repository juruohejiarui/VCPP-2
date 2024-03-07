#include "../includes/gate.h"
/*

*/

inline void setIntrGate(unsigned int n,unsigned char ist,void * addr)
{
	setGate(IDT_Table + n , 0x8E , ist , addr);	//P,DPL=0,TYPE=E
}

/*

*/

inline void setTrapGate(unsigned int n,unsigned char ist,void * addr)
{
	setGate(IDT_Table + n , 0x8F , ist , addr);	//P,DPL=0,TYPE=F
}

/*

*/

inline void setSystemGate(unsigned int n,unsigned char ist,void * addr)
{
	setGate(IDT_Table + n , 0xEF , ist , addr);	//P,DPL=3,TYPE=F
}

/*

*/

inline void setSystemIntrGate(unsigned int n,unsigned char ist,void * addr)	//int3
{
	setGate(IDT_Table + n , 0xEE , ist , addr);	//P,DPL=3,TYPE=E
}


/*

*/

void setTSS64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
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