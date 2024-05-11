#include "../includes/lib.h"
#include "../includes/log.h"
#include "../includes/memory.h"
#include "../includes/task.h"
#include "gate.h"

extern void divideError();
extern void debug();
extern void nmi();
extern void int3();
extern void overflow();
extern void bounds();
extern void undefinedOpcode();
extern void devNotAvailable();
extern void doubleFault();
extern void coprocessorSegmentOverrun();
extern void invalidTSS();
extern void segmentNotPresent();
extern void stackSegmentFault();
extern void generalProtection();
extern void pageFault();
extern void x87FPUError();
extern void alignmentCheck();
extern void machineCheck();
extern void simdError();
extern void virtualizationError();

void doDivideError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doDebug(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_debug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doNMI(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doInt3(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_int3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doOverflow(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_overflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doBounds(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_bounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doUndefinedOpcode(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doDevNotAvailable(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_device_not_available(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doDoubleFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 *q = NULL;
	p = (u64 *)(rsp + 0x98);
	q = (u64 *)(rsp + 0xa0);
	printk(RED,BLACK,"do_double_fault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CS:%#018lx,FLAGS:%#018lx\n",errorCode , rsp , *p , *q , *(q + 1));
	while(1);
}

void doCoprocessorSegmentOverrun(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_coprocessor_segment_overrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
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

void doSegmentNotPresent(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_segment_not_present(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);

	if(errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

	if((errorCode & 0x02) == 0)
		if(errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

	printk(RED,BLACK,"Segment Selector Index:%#010x\n",errorCode & 0xfff8);

	while(1);
}

void doStackSegmentFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_stack_segment_fault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
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

void doGeneralProtection(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
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
	printk(WHITE, BLACK, "registers: \n");
	for (int i = 0; i < 26; i++)
		printk(WHITE, BLACK, "%#04lx(%%rsp) = %#018lx%c", i * 8, *(u64 *)(rsp + i * 8), (i + 1) % 4 == 0 ? '\n' : ' ');
	u64 orgRsp = *(u64 *)(rsp + 0xB0);
	printk(WHITE, BLACK, "orgRsp = %#018lx\n", orgRsp);
	for (int i = 0; i <= 6; i++) {
		printk(WHITE, BLACK, "%#04lx(%%orgRsp) = %#018lx%c", i * 8, *(u64 *)(orgRsp + i * 8), (i + 1) % 4 == 0 ? '\n' : ' ');
	}
	while(1);
}

u64 doPageFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 cr2 = 0;
	__asm__ volatile("movq %%cr2, %0":"=r"(cr2)::"memory");
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CR2:%#018lx\n",errorCode , rsp , *p , cr2);
	// __asm__ volatile("movq %%cr2, %0":"=r"(cr2)::"memory");
	u64 pldEntry = PageTable_getPldEntry(getCR3(), cr2);
	// blank pldEntry means the page is not mapped
	
	if (pldEntry == 0) {
		for (int i = 0; i < sizeof(PtReg) / sizeof(u64); i++)
			printk(WHITE, BLACK, "%#04lx(%%rsp) = %#018lx%c", i * 8, *(u64 *)(rsp + i * 8), (i + 1) % 4 == 0 ? '\n' : ' ');
		printk(WHITE, BLACK, "\n");
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
	// map this virtual address without physics page
	Page *page = Buddy_alloc(0, Page_Flag_Active);
	// __asm__ volatile("movq %%cr2, %0":"=r"(cr2)::"memory");
	PageTable_map(getCR3(), cr2 & ~0xfff, page->phyAddr);
	// printk(WHITE, BLACK, "rflag = %#018lx\n", IO_getRflags());
	return 0;
}

void doX87FPUError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_x87_fpu_error(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doAlignmentCheck(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_alignment_check(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doMachineCheck(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_machine_check(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doSIMDError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_simd_error(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void doVirtualizationError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_virtualization_exception(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void Init_systemVector() {
    Gate_setTrap(0, 1, divideError);
	Gate_setTrap(1, 1, debug);
	Gate_setIntr(2, 1, nmi);
	Gate_setSystem(3, 1, int3);
	Gate_setSystem(4, 1, overflow);
	Gate_setSystem(5, 1, bounds);
	Gate_setTrap(6, 1, undefinedOpcode);
	Gate_setTrap(7, 1, devNotAvailable);
	Gate_setTrap(8, 1, doubleFault);
	Gate_setTrap(9, 1, coprocessorSegmentOverrun);
	Gate_setTrap(10, 1, invalidTSS);
	Gate_setTrap(11, 1, segmentNotPresent);
	Gate_setTrap(12, 1, stackSegmentFault);
	Gate_setTrap(13, 1, generalProtection);
	Gate_setTrap(14, 1, pageFault);
	// 15 reserved
	Gate_setTrap(16, 1, x87FPUError);
	Gate_setTrap(17, 1, alignmentCheck);
	Gate_setTrap(18, 1, machineCheck);
	Gate_setTrap(19, 1, simdError);
	Gate_setTrap(20, 1, virtualizationError);
}