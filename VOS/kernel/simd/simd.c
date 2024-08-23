#include "../includes/simd.h"
#include "../includes/hardware.h"
#include "../includes/log.h"
#include "../includes/memory.h"
#include "../includes/smp.h"

static u64 _xsaveAreaSize;
static u32 _avxOffset;

void SIMD_init() {
	u32 a, b, c, d;
	_xsaveAreaSize = 0;
	HW_CPU_cpuid(0x01, 0, &a, &b, &c, &d);
	if (!(c & (1u << 26))) { printk(RED, BLACK, "SIMD: no xsave support.\n"); return ; }
	if (!(c & (1u << 28))) { printk(RED, BLACK, "SIMD: no AVX support.\n"); return ; }
	
	// enable xsave and avx
	 u64 cr4 = IO_getCR(4), xcr0 = 0;
    // set bit 18 of cr4 to enable xsave
    IO_setCR(4, cr4 | (1ul << 18));

	HW_CPU_cpuid(0x0d, 0x00, &a, &b, &c, &d);
	printk(WHITE, BLACK, "SIMD: extend state: %#010lx\n", a);

	u32 enbl = (1ul << 1) | (1ul << 2);

	HW_CPU_cpuid(0x07, 0x00, &a, &b, &c, &d);
	if (!(b & (1u << 16))) printk(YELLOW, BLACK, "SIMD: no AVX-512 support.\n");
	else enbl |= (1ul << 5) | (1ul << 6) | (1ul << 7);
	
	xcr0 = IO_getXCR(0);
    IO_setXCR(0, xcr0 | enbl);
    xcr0 = IO_getXCR(0);

	// get _avxOffset and _xsaveAreaSize
	HW_CPU_cpuid(0x0d, 0x02, &a, &b, &c, &d);
	_avxOffset = b;
	HW_CPU_cpuid(0x0d, 0x00, &a, &b, &c, &d);
	_xsaveAreaSize = b;
	printk(WHITE, BLACK, "SIMD: avx offset:%u xsave area size:%ld\n", _avxOffset, _xsaveAreaSize);
}
u64 SIMD_XsaveAreaSize() { return _xsaveAreaSize; }

SIMD_XsaveArea *SIMD_allocXsaveArea(u64 kmallocArg, void (*destructor)(void *)) {
	return kmalloc(_xsaveAreaSize, kmallocArg | Slab_kmalloc_arg_Clear, destructor);
}

// switch the SIMD registers of the current CPU to the current task
void SIMD_switchToCur() {
	SMP_CPUInfoPkg *info = SMP_current;
	SIMD_xsave(info->simdRegDomain->simdRegs);
	info->simdRegDomain = Task_currentDMAS();
	SIMD_xrstor(Task_current->simdRegs);
	Task_current->flags |= Task_Flag_UseFloat;
}

