#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static MADTDescriptor *_madt;

int SMP_x2APICIdx = 0;

SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];
u32 _cpuApicId[Hardware_CPUNumber];

SpinLock _lock;
u32 SMP_cpuNum, trIdxCnt;

static u32 _cvtId(u32 topoIdx) {
	// SMP not enabled
	if (!SMP_cpuNum) return 0;
	int l = 0, r = SMP_cpuNum - 1;
	while (l <= r) {
		int mid = (l + r) >> 1, idx = SMP_cpuInfo[mid].cpuId;
		if (idx == topoIdx) return mid;
		if (idx < topoIdx) l = mid + 1;
		else r = mid - 1;
	}
	return (u32)-1;
}

u32 SMP_registerCPU(u32 topoIdx) {
    SpinLock_lock(&_lock);
	SMP_CPUInfoPkg *pkg = &SMP_cpuInfo[SMP_cpuNum++];
	pkg->cpuId = topoIdx;
    SpinLock_unlock(&_lock);
    // is BSP
    if (SMP_cpuNum == 1)
        pkg->tssTable = tss64Table;
    else {
		u32 trIdx = trIdxCnt;
		trIdxCnt += 2;
        pkg->tssTable = kmalloc(128, Slab_kmalloc_arg_Clear, NULL);
        pkg->trIdx = trIdx;
		pkg->initStk = kmalloc(Init_taskStackSize, 0, NULL);
		Intr_Gate_setTSSDesc(pkg->trIdx, pkg->tssTable);
    }
    return SMP_cpuNum - 1;
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
				printk(WHITE, BLACK, "idx:%d pkg:%#018lx cpuId:%#018lx stack: %#018lx\n", idx, &SMP_cpuInfo[idx], SMP_cpuInfo[idx].cpuId, SMP_cpuInfo[idx].initStk);
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

IntrHandlerDeclare(SMP_irq0xc8Handler) {

}

void SMP_init() {
	printk(RED, BLACK, "SMP_init()\n");
	SpinLock_init(&_lock);
    trIdxCnt = 12;
	// find the local processor list and register each of them.
	int res = _parseMADT();
	if (!res) { printk(RED, BLACK, "SMP: unable to get the processor map.\n"); return ; }


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

	for (int i = 1; i < SMP_cpuNum; i++) {
		icr.vector = 0x20;
		icr.deliverMode = HW_APIC_DeliveryMode_Startup;
		icr.DestShorthand = HW_APIC_DestShorthand_None;
		icr.dest.x2Apic = SMP_cpuInfo[i].cpuId;
		
		IO_writeMSR(0x830, *(u64 *)&icr);
		IO_writeMSR(0x830, *(u64 *)&icr);
		while (!(SMP_cpuInfo[i].flags & SMP_CPUInfo_flag_APUInited))
			IO_hlt();
	}

	

	for (int i = 1; i < SMP_cpuNum; i++) {
		SMP_sendIPI(&SMP_cpuInfo[i], 0xc8);
	}
}

void SMP_sendIPI(SMP_CPUInfoPkg *cpu, u32 vector) {
	APIC_ICRDescriptor icr;
	*(u64 *)&icr = 0;
	icr.vector = vector;
	icr.deliverMode = HW_APIC_DeliveryMode_Fixed;
	icr.destMode = HW_APIC_DestMode_Physical;
	icr.deliverMode = HW_APIC_DeliveryStatus_Idle;
	icr.triggerMode = HW_APIC_TriggerMode_Edge;
	icr.DestShorthand = HW_APIC_DestShorthand_None;
	icr.dest.x2Apic = cpu->cpuId;
	IO_writeMSR(0x830, *(u64 *)&icr);
}

u32 SMP_getCurCPUIndex() {
	u32 a, b, c, d;
    HW_CPU_cpuid(0xb, 0, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &SMP_cpuInfo[idx]; }

