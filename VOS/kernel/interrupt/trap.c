#include "../includes/lib.h"
#include "../includes/log.h"
#include "../includes/memory.h"
#include "../includes/task.h"
#include "gate.h"

extern volatile int Global_state;

char *_regName[] = {
	"r15", "r14", "r13", "r12", "r11", "r10", "r9", "r8",
	"rbx", "rcx", "rdx", "rsi", "rdi", "rbp",
	"ds", "es",
	"rax",
	"func", "error",
	"rip", "cs", "rflags", "rsp", "ss"
};

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

static int _lookupKallsyms(u64 address, int level)
{
	int index = 0;
	int level_index = 0;
	i8 *string = (i8 *)&kallsyms_names;
	for(index = 0;index < kallsyms_syms_num;index++)
		if(address >= kallsyms_addresses[index] && address < kallsyms_addresses[index+1])
			break;
	if(index < kallsyms_syms_num)
	{
		for(level_index = 0;level_index < level;level_index++)
			printk(RED,BLACK,"  ");
		printk(RED,BLACK,"+---> ");

		printk(YELLOW,BLACK,"address:%#018lx \t(+) %04d function:%s\n",address,address - kallsyms_addresses[index],&string[kallsyms_index[index]]);
		return 0;
	}
	else
		return 1;
}

void _backtrace(PtReg *regs)
{
	u64 *rbp = (u64 *)regs->rbp;
	u64 ret_address = regs->rip;
	int i = 0;

	printk(RED,BLACK,"====================== Task Struct Information =====================\n");
	printk(RED,BLACK,"regs->rsp:%#018lx,current->thread->rsp0:%#018lx,current:%#018lx\n",
		regs->rsp, Task_current->thread->rsp0, Task_current);
	printk(RED,BLACK,"====================== Kernel Stack Backtrace ======================\n");

	for(i = 0;i < 10;i++)
	{
		if (_lookupKallsyms(ret_address, i))
			break; 
		if ((u64)rbp < (u64)regs->rsp || (u64)rbp > Task_current->thread->rsp0)
			break;

		ret_address = *(rbp + 1);
		rbp = (u64 *)*rbp;
	}
}

static void _printRegs(u64 rsp) {
	printk(WHITE, BLACK, "registers: \n");
	for (int i = 0; i < sizeof(PtReg) / sizeof(u64); i++)
		printk(WHITE, BLACK, "%6s = %#018lx%c", _regName[i], *(u64 *)(rsp + i * 8), (i + 1) % 8 == 0 ? '\n' : ' ');
	if (Global_state == 1) _backtrace((PtReg *)rsp);
}

void doDivideError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\t" ,errorCode, rsp, *p);
	if (Global_state == 1) {
		printk(WHITE, BLACK, "pid = %d\n", Task_current->pid);
		Task_current->priority = Task_Priority_Trapped;
	} else {
		printk(WHITE, BLACK, "\n");
		_printRegs(rsp);
		while (1) IO_hlt();
	}
	IO_sti();
	while(1) IO_hlt();
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
	printk(RED,BLACK,"do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\t",errorCode , rsp , *p);
	if (Global_state) printk(WHITE, BLACK, "pid = %ld\n", Task_current->pid);
	else printk(WHITE, BLACK, "\n");
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
	_printRegs(rsp);
	while(1) IO_hlt();
}

u64 doPageFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 cr2 = 0;
	__asm__ volatile("movq %%cr2, %0":"=r"(cr2)::"memory");
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CR2:%#018lx\t",errorCode , rsp , *p , cr2);
	if (Global_state) printk(WHITE, BLACK, "pid = %ld\n", Task_current->pid);
	else printk(WHITE, BLACK, "\n");
	u64 pldEntry = MM_PageTable_getPldEntry_debug(getCR3(), cr2);
	// only has attributes
	if ((pldEntry & ~0xffful) == 0 && (pldEntry & 0xffful)) {
		// map this virtual address without physics page
		Page *page = MM_Buddy_alloc(0, Page_Flag_Active);
		MM_PageTable_map(getCR3(), cr2 & ~0xfff, page->phyAddr, pldEntry | MM_PageTable_Flag_Presented);
	} else {
		// blank pldEntry means the page is not mapped
		_printRegs(rsp);
		printk(RED, BLACK, "Invalid entry : %#018lx\n", pldEntry);
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
		while(1) IO_hlt();
	}
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

void Intr_Trap_setSysVec() {
    Intr_Gate_setTrap(0, 1, divideError);
	Intr_Gate_setTrap(1, 1, debug);
	Intr_Gate_setIntr(2, 1, nmi);
	Intr_Gate_setSystem(3, 1, int3);
	Intr_Gate_setSystem(4, 1, overflow);
	Intr_Gate_setSystem(5, 1, bounds);
	Intr_Gate_setTrap(6, 1, undefinedOpcode);
	Intr_Gate_setTrap(7, 1, devNotAvailable);
	Intr_Gate_setTrap(8, 1, doubleFault);
	Intr_Gate_setTrap(9, 1, coprocessorSegmentOverrun);
	Intr_Gate_setTrap(10, 1, invalidTSS);
	Intr_Gate_setTrap(11, 1, segmentNotPresent);
	Intr_Gate_setTrap(12, 1, stackSegmentFault);
	Intr_Gate_setTrap(13, 1, generalProtection);
	Intr_Gate_setTrap(14, 1, pageFault);
	// 15 reserved
	Intr_Gate_setTrap(16, 1, x87FPUError);
	Intr_Gate_setTrap(17, 1, alignmentCheck);
	Intr_Gate_setTrap(18, 1, machineCheck);
	Intr_Gate_setTrap(19, 1, simdError);
	Intr_Gate_setTrap(20, 1, virtualizationError);
}