#include "../includes/lib.h"
#include "../includes/log.h"
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
    setTrapGate(0, 1, divideError);
	setTrapGate(1, 1, debug);
	setIntrGate(2, 1, nmi);
	setSystemGate(3, 1, int3);
	setSystemGate(4, 1, overflow);
	setSystemGate(5, 1, bounds);
	setTrapGate(6, 1, undefinedOpcode);
	setTrapGate(7, 1, devNotAvailable);
	setTrapGate(8, 1, doubleFault);
	setTrapGate(9, 1, coprocessorSegmentOverrun);
	setTrapGate(10, 1, invalidTSS);
	setTrapGate(11, 1, segmentNotPresent);
	setTrapGate(12, 1, stackSegmentFault);
	setTrapGate(13, 1, generalProtection);
	setTrapGate(14, 1, pageFault);
	// 15 reserved
	setTrapGate(16, 1, x87FPUError);
	setTrapGate(17, 1, alignmentCheck);
	setTrapGate(18, 1, machineCheck);
	setTrapGate(19, 1, simdError);
	setTrapGate(20, 1, virtualizationError);
}