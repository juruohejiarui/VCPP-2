#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static MADTDescriptor *_madt;

static int _x2APICIdx = 0;

SMP_CPUInfoPkg _cpuInfo[Hardware_CPUNumber];

SpinLock _lock;
u32 idxCnt, trIdxCnt;

static u32 _cvtId(u32 idx) { return idx; }

u32 SMP_registerCPU(u32 topoIdx) {
    SpinLock_lock(&_lock);
    u32 idx = idxCnt++;
    u32 trIdx = trIdxCnt;
    SpinLock_unlock(&_lock);
    trIdxCnt += 2;
	_cpuInfo[idx].topoIdx = topoIdx;
    // is BSP
    if (!topoIdx)
        _cpuInfo[idx].tssTable = tss64Table;
    else {
        _cpuInfo[idx].tssTable = kmalloc(128, Slab_kmalloc_arg_Clear, NULL);
        _cpuInfo[idx].trIdx = trIdx;
    }
    return idx;
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
					printk(WHITE, BLACK, "Type0: processorID:%d apicId:%d\n", entry->ct.type0.processorID, entry->ct.type0.apicID);
					offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type0);
					// register this processor
					SMP_registerCPU(entry->ct.type0.processorID);
					break;
				case 9:
					printk(WHITE, BLACK, "Type9: x2apicId:%d acpiID:%d\n", entry->ct.type9.x2apicID, entry->ct.type9.acpiID);
					offset += sizeof(u8) * 2 + sizeof(struct MADTEntry_Type9);
				#define skip(typeId) \
				case typeId: \
					printk(WHITE, BLACK, "Type"#typeId"\n"); \
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
	}
}

void SMP_init() {
    for (int i = 0; ; i++) {
        u32 a, b, c, d;
        HW_CPU_cpuid(0xb, i, &a, &b, &c, &d);
        if (((c >> 8) & 0xff) == 0) {
            printk(WHITE, BLACK, "SMP: x2 APIC: level:%d current logical processor:%d\n", c & 0xff, d);
            _x2APICIdx = i;
            break;
        }
        printk(WHITE, BLACK, "SMP: local APIC: type:%d width:%d logical processor:%d\n",
            (c >> 8) & 0xff, a & 0x1f, b & 0xff);
    }

	// find the local processor list and register each of them.
	_parseMADT();

    memset(_cpuInfo, 0, sizeof(_cpuInfo));
    idxCnt = 0, trIdxCnt = 10;
    SpinLock_init(&_lock);
    SMP_registerCPU(SMP_getCurCPUIndex());

    printk(WHITE, BLACK, "SMP: copy byte:%#010lx\n", (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
    memcpy(SMP_APUBootStart, DMAS_phys2Virt(0x20000), (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);

	*(u8 *)DMAS_phys2Virt(0x20000) = 0xf4;

	IO_writeMSR(0x830, 0xc4500);
	IO_writeMSR(0x830, 0xc4620);
	IO_writeMSR(0x830, 0xc4620);
}

u32 SMP_getCurCPUIndex() {
    u32 a, b, c, d;
    HW_CPU_cpuid(0xb, _x2APICIdx, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &_cpuInfo[idx]; }