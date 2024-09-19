#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static MADTDescriptor *_madt;

int SMP_x2APICIdx = 0;

SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];
u32 _cpuApicId[Hardware_CPUNumber];

SpinLock _lock;
int SMP_cpuNum;
u32 trIdxCnt;
Atomic SMP_task0LaunchNum;

static u32 _cvtId(u32 topoIdx) {
	// SMP not enabled
	if (SMP_cpuNum < 1) return 0;
	int l = 0, r = SMP_cpuNum - 1;
	while (l <= r) {
		int mid = (l + r) >> 1, idx = SMP_cpuInfo[mid].apicID;
		if (idx == topoIdx) return mid;
		if (idx < topoIdx) l = mid + 1;
		else r = mid - 1;
	}
	return (u32)-1;
}

u32 SMP_registerCPU(u32 topoIdx) {
	if (SMP_cpuNum > 0 && !topoIdx) return (u32)-1;
    SpinLock_lock(&_lock);
	SMP_CPUInfoPkg *pkg = &SMP_cpuInfo[SMP_cpuNum++];
	memset(pkg, 0, sizeof(SMP_CPUInfoPkg));
	pkg->apicID = topoIdx;
    SpinLock_unlock(&_lock);
    // is BSP
    if (SMP_cpuNum == 1) {
        pkg->tssTable = tss64Table;
		pkg->idtTblSize = 512 * 8;
		pkg->idtTable = idtTable;
		// mask the traps and interrupts
		SMP_maskIntr(0, 0, 0x40);
		SMP_maskIntr(0, 0x80, 0x80);
		// for each processor, there are 64 spare vectors for pci and other purposes. from 0x40 to 0x7f
		
    } else {
		u32 trIdx = trIdxCnt;
		trIdxCnt += 2;
        pkg->tssTable = kmalloc(128, Slab_Flag_Clear, NULL);
        pkg->trIdx = trIdx;
		pkg->initStk = kmalloc(Init_taskStackSize, 0, NULL);
		pkg->idtTblSize = 512 * 8;
		pkg->idtTable = kmalloc(512 * 8, 0, NULL);
		memcpy(idtTable, pkg->idtTable, 512 * 8);
		memcpy(SMP_cpuInfo[0].intrMsk, pkg->intrMsk, sizeof(u64) * 4);
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
				offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type0);
				// register this processor
				int idx = SMP_registerCPU(entry->ct.type0.apicID);
				break;
			case 9 :
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
	printk(WHITE, BLACK, "SMP: processor num: %d\n", SMP_cpuNum);
	return 1;
}

// schedule interrupt
IntrHandlerDeclare(SMP_irq0xc8Handler) {
	if (SMP_current->flags & SMP_CPUInfo_flag_InTaskLoop) Task_updateCurState();
}

void SMP_init() {
	IO_cli();
	printk(RED, BLACK, "SMP_init()\n");
	SpinLock_init(&_lock);
	SMP_cpuNum = 0;
    trIdxCnt = 12;
	// find the local processor list and register each of them.
	int res = _parseMADT();
	if (!res) { printk(RED, BLACK, "SMP: unable to get the processor map.\n"); return ; }
	IO_sti();


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
	printk(WHITE, BLACK, "SMP: init-IPI sent\n"); 

	for (int i = 1; i < SMP_cpuNum; i++) {
		icr.vector = 0x20;
		icr.deliverMode = HW_APIC_DeliveryMode_Startup;
		icr.DestShorthand = HW_APIC_DestShorthand_None;
		icr.dest.x2Apic = SMP_cpuInfo[i].apicID;
		
		IO_writeMSR(0x830, *(u64 *)&icr);
		IO_writeMSR(0x830, *(u64 *)&icr);
		while (!(SMP_cpuInfo[i].flags & SMP_CPUInfo_flag_APUInited))
			IO_hlt();
	}
	printk(WHITE, BLACK, "SMP: startUp-IPI sent\n");
	MM_PageTable_cleanTmpMap();
	Intr_register(0xc8, NULL, SMP_irq0xc8Handler, 0, NULL, "SMP IPI 0xc8");
}

void SMP_sendIPI(int cpuId, u32 vector, void *msg) {
	SMP_cpuInfo[cpuId].ipiMsg = msg;
	APIC_ICRDescriptor icr;
	*(u64 *)&icr = 0;
	icr.vector = vector;
	icr.level = HW_APIC_Level_Assert;
	icr.deliverMode = HW_APIC_DeliveryMode_Fixed;
	icr.destMode = HW_APIC_DestMode_Physical;
	icr.triggerMode = HW_APIC_TriggerMode_Edge;
	icr.DestShorthand = HW_APIC_DestShorthand_None;
	icr.dest.x2Apic = SMP_cpuInfo[cpuId].apicID;
	IO_writeMSR(0x830, *(u64 *)&icr);
}

void SMP_sendIPI_all(u32 vector, void *msg) {
	for (int i = 0; i < SMP_cpuNum; i++)
		SMP_cpuInfo[i].ipiMsg = msg;
	APIC_ICRDescriptor icr;
	*(u64 *)&icr = 0;
	icr.vector = vector;
	icr.level = HW_APIC_Level_Assert;
	icr.deliverMode = HW_APIC_DeliveryMode_Fixed;
	icr.destMode = HW_APIC_DestMode_Physical;
	icr.triggerMode = HW_APIC_TriggerMode_Edge;
	icr.DestShorthand = HW_APIC_DestShorthand_AllIncludingSelf;
	IO_writeMSR(0x830, *(u64 *)&icr);
}
void SMP_sendIPI_self(u32 vector, void *msg) {
	SMP_sendIPI(SMP_getCurCPUIndex(), vector, msg);
}

void SMP_sendIPI_allButSelf(u32 vector, void *msg) {
	int self = SMP_getCurCPUIndex();
	for (int i = 0; i < SMP_cpuNum; i++) 
		if (i != self)
			SMP_cpuInfo[i].ipiMsg = msg;
	APIC_ICRDescriptor icr;
	*(u64 *)&icr = 0;
	icr.vector = vector;
	icr.level = HW_APIC_Level_Assert;
	icr.deliverMode = HW_APIC_DeliveryMode_Fixed;
	icr.destMode = HW_APIC_DestMode_Physical;
	icr.triggerMode = HW_APIC_TriggerMode_Edge;
	icr.DestShorthand = HW_APIC_DestShorthand_AllExcludingSelf;
	IO_writeMSR(0x830, *(u64 *)&icr);
}

u32 SMP_getCurCPUIndex() {
	u32 a, b, c, d;
    HW_CPU_cpuid(0xb, 0, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &SMP_cpuInfo[idx]; }

void SMP_maskIntr(int cpuId, u8 vecSt, u8 vecNum) {
	u64 *mask = SMP_cpuInfo[cpuId].intrMsk;
	for (int i = vecSt; i < vecSt + vecNum; i++)
		mask[i / 64] |= (1ul << (i % 64));
}

int SMP_testIntr(int cpuId, u8 vecSt, u8 vecNum) {
	u64 *mask = SMP_cpuInfo[cpuId].intrMsk;
	for (int i = vecSt; i < vecSt + vecNum; i++)
		if (mask[i / 64] & (1ul << (i % 64))) return i; 
	return -1;
}

void SMP_allocIntrVec(int num, int *cpuId, u8 *vecSt) {
	for (int i = 0; i < SMP_cpuNum; i++) {
		for (int st = 0, err; st + num - 1 <= 0xff; st++)
			if ((err = SMP_testIntr(i, st, num)) == -1) {
				SMP_maskIntr(i, st, num);
				*cpuId = i, *vecSt = st;
				return ;
			} else st = err;
	}
	*cpuId = -1;
}

void SMP_freeIntrVec(int cpuId, int vecSt, int vecNum) {
	u64 *mask = SMP_cpuInfo[cpuId].intrMsk;
	for (int i = vecSt; i < vecSt + vecNum; i++) mask[i / 64] &= ~(1ul << (i % 64));
}