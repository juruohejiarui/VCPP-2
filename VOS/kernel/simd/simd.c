#include "../includes/simd.h"
#include "../includes/hardware.h"
#include "../includes/log.h"
#include "../includes/memory.h"
#include "../includes/smp.h"

static u64 _xsaveAreaSize;
static u32 _avxOffset;

void SIMD_init() {
	SIMD_enable();
	u32 a, b, c, d;
	// get _avxOffset and _xsaveAreaSize
	HW_CPU_cpuid(0x0d, 0x02, &a, &b, &c, &d);
	_avxOffset = b;
	HW_CPU_cpuid(0x0d, 0x00, &a, &b, &c, &d);
	_xsaveAreaSize = b;
	printk(WHITE, BLACK, "SIMD: avx offset:%u xsave area size:%ld\n", _avxOffset, _xsaveAreaSize);
}

void SIMD_enable() {
	// printk(WHITE, BLACK, "enable SIMD on processor %d\n", SMP_getCurCPUIndex());
	u32 a, b, c, d;
	HW_CPU_cpuid(0x01, 0, &a, &b, &c, &d);
	if (!(c & (1u << 26))) { printk(RED, BLACK, "SIMD: no xsave support.\n"); while (1) IO_hlt(); return ; }
	if (!(c & (1u << 28))) { printk(RED, BLACK, "SIMD: no AVX support.\n"); while (1) IO_hlt(); return ; }
	// set Monitor Coprocessor flag (bit 1) and Numeric Error flag (bit 5) of cr0
	u64 cr0 = IO_getCR(0);
	IO_setCR(0, cr0 | (1ul << 1) | (1ul << 5) | (1ul << 16));
	// printk(WHITE, BLACK, "SIMD: cr0:%#018lx\n", IO_getCR(0));
	// enable xsave and avx
	u64 cr4 = IO_getCR(4), xcr0 = 0;
    // set bit 18 of cr4 to enable xsave
    IO_setCR(4, cr4 | 0x40668);
	// printk(WHITE, BLACK, "SIMD: cr4:%#018lx\n", IO_getCR(4));

	HW_CPU_cpuid(0x0d, 0x00, &a, &b, &c, &d);
	// printk(WHITE, BLACK, "SIMD: extend state: %#010lx\n", a);

	u32 enbl = (1ul << 1) | (1ul << 2);

	HW_CPU_cpuid(0x07, 0x00, &a, &b, &c, &d);
	if (b & (1u << 16)) enbl |= (1ul << 5) | (1ul << 6) | (1ul << 7);
	// else printk(YELLOW, BLACK, "SIMD: no AVX-512 support.\n");
	
	xcr0 = IO_getXCR(0);
    IO_setXCR(0, xcr0 | enbl);
    xcr0 = IO_getXCR(0);

	// mask all the exception
	u32 mxcsr = SIMD_getMXCSR();
	SIMD_setMXCSR(SIMD_getMXCSR() | ((1ul << 5) | (1ul << 7) | (1ul << 8) | (1ul << 9) | (1ul << 10) | (1ul << 11) | (1ul << 12)));
	// printk(WHITE, BLACK, "mxcsr:%#010x\n", SIMD_getMXCSR());
}

u64 SIMD_XsaveAreaSize() { return _xsaveAreaSize; }

SIMD_XsaveArea *SIMD_allocXsaveArea(u64 kmallocArg, void (*destructor)(void *)) {
	return kmalloc(_xsaveAreaSize, kmallocArg | Slab_Flag_Clear, destructor);
}

// switch the SIMD registers of the current CPU to the current task
void SIMD_switchToCur() {
	SMP_CPUInfoPkg *info = SMP_current;
	if (info->simdRegDomain == Task_currentDMAS()) return ; 
	if (info->simdRegDomain) {
		SIMD_xsave(info->simdRegDomain->simdRegs);
		SIMD_xrstor(Task_current->simdRegs);
	}
	info->simdRegDomain = Task_currentDMAS();
	Task_current->flags |= Task_Flag_UseFloat;
}

