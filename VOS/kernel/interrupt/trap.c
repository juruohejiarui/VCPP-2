#include "../includes/lib.h"
#include "../includes/log.h"
#include "gate.h"

extern void divideError();

void doDivideError(u64 rsp, u64 errorCode)
{
	unsigned long * p = NULL;
	p = (unsigned long *)(rsp + 0x98);
	printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void Init_systemVector() {
    setTrapGate(0, 1, divideError);
}