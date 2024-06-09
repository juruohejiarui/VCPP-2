#include "../PCIe.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"

PCIeDescriptor *_desc;
i32 devCnt;

static inline PCIeDevice *_getDevPtr(u64 addrBase, u8 bus, u8 dev, u8 func) {
    return (PCIeDevice *)DMAS_phys2Virt(addrBase | ((u64)bus << 20) | ((u64)dev << 15) | ((u64)func << 12));
}

static inline u16 _getVendor(u64 addrBase, u8 bus, u8 dev, u8 func) {
    return _getDevPtr(addrBase, bus, dev, func)->vendorID;
}
static inline u8 _getHeaderType(u32 addrBase, u8 bus, u8 dev, u8 func) {
    return _getDevPtr(addrBase, bus, dev, func)->headerType;
}

void _chkBus(u64 addrBase, u8 bus);

void _chkFunc(u64 addrBase, u8 bus, u8 dev, u8 func) {
    if (_getVendor(addrBase, bus, dev, func) == 0xffff) return ;
    PCIeDevice *devPtr = _getDevPtr(addrBase, bus, dev, func);
    if (devPtr->classCode == 0x06 && devPtr->subclass == 0x4) {
        return ;
    }
    printk(YELLOW, BLACK, "%02x-%02x-%02x: Class:%#04x-%#04x progIF:%#04x vendor:%06x, dev:%06x ", 
            bus, dev, func, devPtr->classCode, devPtr->subclass, devPtr->progIF, devPtr->vendorID, devPtr->devID);
    printk(WHITE, BLACK, "%s\n",
            (PCIeClassDesc[devPtr->classCode][devPtr->subclass] == NULL ? "Unknown" : PCIeClassDesc[devPtr->classCode][devPtr->subclass]));
    
}

void _chkDev(u64 addrBase, u8 bus, u8 dev) {
    u16 vendor = _getVendor(addrBase, bus, dev, 0);
    if (vendor == 0xffff) return ;
    u64 head = _getHeaderType(addrBase, bus, dev, 0);
    if ((head & 0x80) == 0) _chkFunc(addrBase, bus, dev, 0);
    else {
        for (int func = 0; func < 8; func++) _chkFunc(addrBase, bus, dev, func);
    }
}

void _chkBus(u64 addrBase, u8 bus) {
    for (u8 dev = 0; dev < 32; dev++) _chkDev(addrBase, bus, dev);
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
            _chkBus(_desc->structs[i].address, bus);
        
    }
}