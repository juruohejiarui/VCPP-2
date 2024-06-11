#include "../PCIe.h"
#include "mgr.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"

static PCIeDescriptor *_desc;
i32 _devCnt;

static List _mgrList;

static inline PCIeConfig *_getDevPtr(u64 addrBase, u8 bus, u8 dev, u8 func) {
    return (PCIeConfig *)DMAS_phys2Virt(addrBase | ((u64)bus << 20) | ((u64)dev << 15) | ((u64)func << 12));
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
    PCIeConfig *devPtr = _getDevPtr(addrBase, bus, dev, func);
    if (devPtr->classCode == 0x06 && devPtr->subclass == 0x4) {
        return ;
    }
    printk(YELLOW, BLACK, "%02x-%02x-%02x: Class:%02x%02x progIF:%02x vendor:%04x dev:%04x header:%02x ", 
            bus, dev, func, devPtr->classCode, devPtr->subclass, devPtr->progIF, devPtr->vendorID, devPtr->devID, devPtr->headerType);
    printk(WHITE, BLACK, "%s\n",
            (PCIeClassDesc[devPtr->classCode][devPtr->subclass] == NULL ? "Unknown" : PCIeClassDesc[devPtr->classCode][devPtr->subclass]));
    
    PCIeManager *mgrStruct = (PCIeManager *)kmalloc(sizeof(PCIeManager), 0);
    memset(mgrStruct, 0, sizeof(PCIeManager));
    List_init(&mgrStruct->listEle);
    mgrStruct->bus = bus;
    mgrStruct->dev = dev;
    mgrStruct->func = func;
    mgrStruct->device = devPtr;
    List_insBefore(&mgrStruct->listEle, &_mgrList);
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

List *HW_PCIe_getMgrList() { return &_mgrList; }

void HW_PCIe_init() {
    _desc = NULL;
    List_init(&_mgrList);
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
    _devCnt = (_desc->header.length - sizeof(PCIeDescriptor)) / sizeof(PCIeStruct);
    for (int i = 0; i < _devCnt; i++) {
        printk(WHITE, BLACK,
                "PCIe device %d: address: %#018lx, segment: %#04x, start bus: %#02x, end bus: %#02x\n", 
                i, _desc->structs[i].address, _desc->structs[i].segment, _desc->structs[i].stBus, _desc->structs[i].edBus);
        for (u8 bus = _desc->structs[i].stBus; bus <= _desc->structs[i].edBus; bus++)
            _chkBus(_desc->structs[i].address, bus);
        
    }
}