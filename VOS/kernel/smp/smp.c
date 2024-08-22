#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static MADTDescriptor *_madt;

int SMP_x2APICIdx = 0;

SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];
u32 _cpuApicId[Hardware_CPUNumber];

SpinLock _lock;
u32 cpuCnt, trIdxCnt;

static u32 _cvtId(u32 idx) { return idx; }

u32 SMP_registerCPU(u32 topoIdx) {
    SpinLock_lock(&_lock);
	SMP_CPUInfoPkg *pkg = &SMP_cpuInfo[topoIdx];
	pkg->cpuId = ++cpuCnt;
	_cpuApicId[pkg->cpuId] = topoIdx;
    SpinLock_unlock(&_lock);
    // is BSP
    if (!topoIdx)
        pkg->tssTable = tss64Table;
    else {
		u32 trIdx = trIdxCnt;
		trIdxCnt += 2;
        pkg->tssTable = kmalloc(128, Slab_kmalloc_arg_Clear, NULL);
        pkg->trIdx = trIdx;
		pkg->initStk = kmalloc(Init_taskStackSize, 0, NULL);
		Intr_Gate_setTSSDesc(pkg->trIdx, pkg->tssTable);
    }
    return cpuCnt;
}

static int _parseMADT() {
	XSDTDescriptor *xsdt = HW_UEFI_getXSDT();
	_madt = NULL;
	for (int i = 0; i < (xsdt->header.length - sizeof(XSDTDescriptor)) / 8; i++) {
		ACPIHeader *hdr = DMAS_phys2Virt(xsdt->entry[i]);
		if (strncmp(hdr->signature, "APIC", 4) != 0) continue;
		_madt = container(hdr, MADTDescriptor, header);
		break;
	}
	if (_madt == NULL) {
		printk(WHITE, BLACK, "SMP: no madt.\n");
		return 0;
	} else printk(WHITE, BLACK, "SMP: madt:%#018lx length:%ld\n", _madt, _madt->header.length);

	for (u64 offset = sizeof(MADTDescriptor); offset < _madt->header.length; ) {
		struct MADTEntry *entry = (struct MADTEntry *)((u64)_madt + offset);
		switch (entry->type) {
			case 0 :
				printk(WHITE, BLACK, "Type0: processorID:%d apicId:%d\t", entry->ct.type0.processorID, entry->ct.type0.apicID);
				offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type0);
				// register this processor
				int idx = SMP_registerCPU(entry->ct.type0.apicID);
				printk(WHITE, BLACK, "idx:%d cpuInfo:%#018lx stack: %#018lx\n", idx, &SMP_cpuInfo[entry->ct.type0.apicID], SMP_cpuInfo[entry->ct.type0.apicID].initStk);
				break;
			case 9 :
				printk(WHITE, BLACK, "Type9: x2apic:%d apicId:%d\n", entry->ct.type9.x2apicID, entry->ct.type9.apicID);
				offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type9);
				break;
			#define skip(typeId) \
			case typeId: \
				offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type##typeId); \
				break;
			skip(1)
			skip(2)
			skip(3)
			skip(4)
			skip(5)
			#undef skip
		}
	}
	return 1;
}

void SMP_init() {
	printk(RED, BLACK, "SMP_init()\n");
	memset(SMP_cpuInfo, 0, sizeof(SMP_cpuInfo));
    cpuCnt = 0, trIdxCnt = 12;
	// find the local processor list and register each of them.
	int res = _parseMADT();
	if (!res) { printk(RED, BLACK, "SMP: unable to get the processor map.\n"); return ; }

    SpinLock_init(&_lock);

    printk(WHITE, BLACK, "SMP: copy byte:%#010lx\n", (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
    memcpy(SMP_APUBootStart, DMAS_phys2Virt(0x20000), (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
	
	APIC_ICRDescriptor icr;
	*(u64 *)&icr = 0;
	icr.vector = 0x00;
	icr.deliverMode = HW_APIC_DeliveryMode_INIT;
	icr.destMode = HW_APIC_DestMode_Physical;
	icr.deliverStatus = HW_APIC_DeliveryStatus_Idle;
	icr.level = HW_APIC_Level_Assert;
	icr.triggerMode = HW_APIC_TriggerMode_Edge;
	icr.DestShorthand = HW_APIC_DestShorthand_AllExcludingSelf;
	icr.dest.x2Apic = 0;
	IO_writeMSR(0x830, *(u64 *)&icr);

	for (int i = 2; i <= cpuCnt; i++) {
		icr.vector = 0x20;
		icr.deliverMode = HW_APIC_DeliveryMode_Startup;
		icr.DestShorthand = HW_APIC_DestShorthand_None;
		icr.dest.x2Apic = _cpuApicId[i];
		
		IO_writeMSR(0x830, *(u64 *)&icr);
		IO_writeMSR(0x830, *(u64 *)&icr);
		while (!(SMP_cpuInfo[_cpuApicId[i]].flags & SMP_CPUINfo_flag_APUInited))
			IO_hlt();
	}
}

u32 SMP_getCurCPUIndex() {
    u32 a, b, c, d;
    HW_CPU_cpuid(0xb, 0, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &SMP_cpuInfo[idx]; }

void startSMP() {
	u64 rsp = 0;
	SMP_CPUInfoPkg *pkg = SMP_getCPUInfoPkg(SMP_getCurCPUIndex());
	rsp = (u64)pkg->initStk + Init_taskStackSize;	
	u32 x, y;
	__asm__ volatile (
		"movq $0x1b, %%rcx		\n\t"
		"rdmsr 					\n\t"
		"bts $10, %%rax			\n\t"
		"bts $11, %%rax			\n\t"
		"wrmsr					\n\t"
		"movq $0x1b, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory", "rcx"
	);

	__asm__ volatile (
		"movq $0x80f, %%rcx		\n\t"
		"rdmsr					\n\t"
		"bts $8, %%rax			\n\t"
		"bts $12, %%rax			\n\t"
		"wrmsr					\n\t"
		"movq $0x80f, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory", "rcx"
	);
	__asm__ volatile (
		"movq $0x802, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory"
	);
	// half of the initStk to be the trap stack
	rsp -= 0x4000ul;
	Intr_Gate_setTSS(pkg->tssTable, rsp + 0x4000ul, rsp + 0x4000ul, rsp + 0x4000ul, rsp, rsp, rsp, rsp, rsp, rsp, rsp);
	Intr_Gate_loadTR(pkg->trIdx);
	printk(WHITE, BLACK, "APU %d: tr:%d trap rsp:%#018lx\n", SMP_getCurCPUIndex(), pkg->trIdx, rsp);
	SMP_current->flags |= SMP_CPUINfo_flag_APUInited;
	x = 1 / 0;
	IO_hlt();
}