#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static MADTDescriptor *_madt;

int SMP_x2APICIdx = 0;

SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];

SpinLock _lock;
u32 cpuCnt, trIdxCnt;

static u32 _cvtId(u32 idx) { return idx; }

u32 SMP_registerCPU(u32 topoIdx) {
    SpinLock_lock(&_lock);
    u32 trIdx = trIdxCnt;
	trIdxCnt += 2;
	SMP_CPUInfoPkg *pkg = &SMP_cpuInfo[topoIdx];
	pkg->cpuId = ++cpuCnt;
    SpinLock_unlock(&_lock);
    // is BSP
    if (!topoIdx)
        pkg->tssTable = tss64Table;
    else {
        pkg->tssTable = kmalloc(128, Slab_kmalloc_arg_Clear, NULL);
        pkg->trIdx = trIdx;
		pkg->initStk = kmalloc(32768, 0, NULL);
    }
    return cpuCnt;
}

static void _parseMADT() {
	XSDTDescriptor *xsdt = HW_UEFI_getXSDT();
	_madt = NULL;
	for (int i = 0; i < (xsdt->header.length - sizeof(XSDTDescriptor)) / 8; i++) {
		ACPIHeader *hdr = DMAS_phys2Virt(xsdt->entry[i]);
		for (int c = 0; c < 4; c++) printk(WHITE, BLACK, "%c", hdr->signature[c]);
		printk(WHITE, BLACK, "\n");
		if (strncmp(hdr->signature, "APIC", 4) != 0) continue;
		_madt = container(hdr, MADTDescriptor, header);
		break;
	}
	if (_madt == NULL) {
		printk(WHITE, BLACK, "SMP: no madt.\n");
		return ;
	} else printk(WHITE, BLACK, "SMP: madt:%#018lx length:%ld\n", _madt, _madt->header.length);

	{
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
				#define skip(typeId) \
				case typeId: \
					offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type##typeId); \
					break;
				skip(1)
				skip(2)
				skip(3)
				skip(4)
				skip(5)
				skip(9)
				#undef skip
			}
		}
	}
}

void SMP_init() {
    for (int i = 0; ; i++) {
        u32 a, b, c, d;
        HW_CPU_cpuid(0xb, i, &a, &b, &c, &d);
        if (((c >> 8) & 0xff) == 0) {
            printk(WHITE, BLACK, "SMP: x2 APIC: level:%d current logical processor:%d\n", c & 0xff, d);
            SMP_x2APICIdx = i;
            break;
        }
        printk(WHITE, BLACK, "SMP: local APIC: type:%d width:%d logical processor:%d\n",
            (c >> 8) & 0xff, a & 0x1f, b & 0xff);
    }
	memset(SMP_cpuInfo, 0, sizeof(SMP_cpuInfo));
    cpuCnt = 0, trIdxCnt = 10;
	// find the local processor list and register each of them.
	_parseMADT();

    SpinLock_init(&_lock);

    printk(WHITE, BLACK, "SMP: copy byte:%#010lx\n", (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
    memcpy(SMP_APUBootStart, DMAS_phys2Virt(0x20000), (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
	
	IO_writeMSR(0x830, 0xc4500);
	IO_writeMSR(0x830, 0xc4620);
	IO_writeMSR(0x830, 0xc4620);
}

u32 SMP_getCurCPUIndex() {
    u32 a, b, c, d;
    HW_CPU_cpuid(0xb, 0, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &SMP_cpuInfo[idx]; }

void startSMP() {
	u64 rsp = 0;
	__asm__ volatile ( "movq %%rsp, %0" : "=a"(rsp) : : "memory");
	printk(WHITE, BLACK, "APU starting... processor id:%d stk:%#018lx\n", SMP_getCurCPUIndex(), rsp);
	IO_hlt();
}