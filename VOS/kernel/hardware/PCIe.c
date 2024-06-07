#include "PCIe.h"
#include "../includes/memory.h"
#include "../includes/log.h"

PCIeDescriptor *_desc;
i32 devCnt;

void _chkBus(u64 addrBase, u8 bus) {
    
}

void HW_PCIe_init() {
    _desc = NULL;
    printk(RED, BLACK, "HW_PCIe_init()\n");
    XSDTDescriptor *xsdt = HW_UEFI_getXSDT();
    u32 entryCnt = (xsdt->header.length - sizeof(XSDTDescriptor)) / 8;
    for (int i = 0; i < entryCnt; i++) {
        ACPIHeader *header = (ACPIHeader *)DMAS_phys2Virt(xsdt->entry[i]);
        if (!strncmp(header->signature, "MCFG", 4)) {
            _desc = (PCIeDescriptor *)header;
            break;
        }
    }
    if (_desc == NULL) { 
        printk(RED, BLACK, "PCIe no found...");
        while (1) IO_hlt();
    }
    printk(WHITE, BLACK, "find PCIeDescriptor at %#018lx\n", _desc);
    devCnt = (_desc->header.length - sizeof(PCIeDescriptor)) / sizeof(PCIeStruct);
    for (int i = 0; i < devCnt; i++) {
        printk(WHITE, BLACK,
                "PCIe device %d: address: %#018lx, segment: %#04x, start bus: %#02x, end bus: %#02x\n", 
                i, _desc->structs[i].address, _desc->structs[i].segment, _desc->structs[i].stBus, _desc->structs[i].edBus);
        for (u8 bus = _desc->structs[i].stBus; bus <= _desc->structs[i].edBus; bus++) 
            _chkBus((u64)DMAS_phys2Virt(_desc->structs[i].address + (bus << 20)), bus);
    }
}