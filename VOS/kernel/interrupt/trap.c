#include "../includes/lib.h"
#include "../includes/log.h"
#include "gate.h"

extern void divideError();
extern void nmi();
extern void undefinedOpcode();
extern void invalidTSS();
extern void pageFault();

void doDivideError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doNMI(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doUndefinedOpcode(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doInvalidTSS(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_invalid_tss(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	if (errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during the delivery of an event external to the program, such as an interrupt or an exception.\n");
	if (errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else {
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");
		if (errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the GDT;\n");
	}
	printk(RED,BLACK,"Segment Selector Index:%#018lx\n",errorCode & 0xfff8);
	while(1);
}

void doPageFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 cr2 = 0;
	__asm__ __volatile__("movq %%cr2, %0":"=r"(cr2)::"memory");
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CR2:%#018lx\n",errorCode , rsp , *p , cr2);
	if (errorCode & 0x01)
		printk(RED,BLACK,"The page fault was caused by a non-present page.\n");
	if (errorCode & 0x02)
		printk(RED,BLACK,"The page fault was caused by a page-level protection violation.\n");
	if (errorCode & 0x04)
		printk(RED,BLACK,"The page fault was caused by a non-present page.\n");
	if (errorCode & 0x08)
		printk(RED,BLACK,"The page fault was caused by a page-level protection violation.\n");
	if (errorCode & 0x10)
		printk(RED,BLACK,"The page fault was caused by reading a reserved bit.\n");
	if (errorCode & 0x20)
		printk(RED,BLACK,"The page fault was caused by an instruction fetch.\n");
	if (errorCode & 0x40)
		printk(RED,BLACK,"The page fault was caused by reading a reserved bit.\n");
	if (errorCode & 0x80)
		printk(RED,BLACK,"The page fault was caused by an instruction fetch.\n");
	while(1);
}

void Init_systemVector() {
    setTrapGate(0, 1, divideError);
	setIntrGate(2, 1, nmi);
	setTrapGate(6, 1, undefinedOpcode);
	setTrapGate(10, 1, invalidTSS);
	setTrapGate(14, 1, pageFault);
}