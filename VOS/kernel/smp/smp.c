#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

static int _x2APICIdx = 0;

SMP_CPUInfoPkg _cpuInfo[Hardware_CPUNumber];

SpinLock _lock;
u32 idxCnt, trIdxCnt;

u32 _cvtId(u32 idx) {
    return 0;
}

u32 SMP_registerCPU(u32 topoIdx) {
    SpinLock_lock(&_lock);
    u32 idx = idxCnt++;
    u32 trIdx = trIdxCnt;
    SpinLock_unlock(&_lock);
    trIdxCnt += 2;
    // is BSP
    if (!topoIdx)
        _cpuInfo[idx].tssTable = tss64Table;
    else {
        _cpuInfo[idx].tssTable = kmalloc(128, Slab_kmalloc_arg_Clear, NULL);
        _cpuInfo[idx].trIdx = trIdx;
    }
    return idx;
}

void SMP_init() {
    for (int i = 0; ; i++) {
        u32 a, b, c, d;
        HW_CPU_getID(0xb, i, &a, &b, &c, &d);
        if (((c >> 8) & 0xff) == 0) {
            printk(WHITE, BLACK, "SMP: x2 APIC: level:%d current logical processor:%d\n", c & 0xff, d);
            _x2APICIdx = i;
            break;
        }
        printk(WHITE, BLACK, "SMP: local APIC: type:%d width:%d logical processor:%d\n",
            (c >> 8) & 0xff, a & 0x1f, b & 0xff);
    }
    memset(_cpuInfo, 0, sizeof(_cpuInfo));
    idxCnt = 0, trIdxCnt = 10;
    SpinLock_init(&_lock);
    SMP_registerCPU(SMP_getCurCPUIndex());

    printk(WHITE, BLACK, "SMP: copy byte:%#010lx\n", (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
    memcpy(SMP_APUBootStart, DMAS_phys2Virt(0x20000), (u64)&SMP_APUBootEnd - (u64)&SMP_APUBootStart);
}

u32 SMP_getCurCPUIndex() {
    u32 a, b, c, d;
    HW_CPU_getID(0xb, _x2APICIdx, &a, &b, &c, &d);
    return _cvtId(d);
}

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx) { return &_cpuInfo[idx]; }