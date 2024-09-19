#include "../PCIe.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"
#include "../../includes/smp.h"

static PCIeDescriptor *_desc;
i32 _devCnt;

static List _mgrList;

IntrController _MSI_defaultController;

static __always_inline__ u16 _getVendor(u64 addrBase, u8 bus, u8 slot, u8 func) {
    return HW_PCIe_getDevPtr(addrBase, bus, slot, func)->vendorID;
}
static __always_inline__ u8 _getHeaderType(u32 addrBase, u8 bus, u8 slot, u8 func) {
    return HW_PCIe_getDevPtr(addrBase, bus, slot, func)->headerType;
}
	
void _chkBus(u64 addrBase, u8 bus);

void _chkFunc(u64 addrBase, u8 bus, u8 slot, u8 func) {
    if (_getVendor(addrBase, bus, slot, func) == 0xffff) return ;
    PCIeConfig *devPtr = HW_PCIe_getDevPtr(addrBase, bus, slot, func);
    if (devPtr->classCode == 0x06 && devPtr->subclass == 0x4) {
        return ;
    }
	#ifdef DEBUG_PCIE
    printk(YELLOW, BLACK, "%02x-%02x-%02x: Class:%02x%02x progIF:%02x vendor:%04x dev:%04x header:%02x ", 
            bus, dev, func, devPtr->classCode, devPtr->subclass, devPtr->progIF, devPtr->vendorID, devPtr->devID, devPtr->headerType);
    printk(WHITE, BLACK, "%s\n",
            (PCIeClassDesc[devPtr->classCode][devPtr->subclass] == NULL ? "Unknown" : PCIeClassDesc[devPtr->classCode][devPtr->subclass]));
	#endif
    
    PCIeManager *mgrStruct = (PCIeManager *)kmalloc(sizeof(PCIeManager), 0, NULL);
    memset(mgrStruct, 0, sizeof(PCIeManager));
    List_init(&mgrStruct->listEle);
    mgrStruct->bus = bus;
    mgrStruct->slot = slot;
    mgrStruct->func = func;
    mgrStruct->cfg = devPtr;
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
		#ifdef DEBUG_PCIE
        printk(WHITE, BLACK,
                "PCIe device %d: address: %#018lx, segment: %#04x, start bus: %#02x, end bus: %#02x\n", 
                i, _desc->structs[i].address, _desc->structs[i].segment, _desc->structs[i].stBus, _desc->structs[i].edBus);
		#endif
        for (u16 bus = (u16)_desc->structs[i].stBus; bus <= (u16)_desc->structs[i].edBus; bus++)
            _chkBus(_desc->structs[i].address, bus);
        
    }
	memset(&_MSI_defaultController, 0, sizeof(IntrController));
	_MSI_defaultController.ack = HW_APIC_edgeAck;
}

void HW_PCIe_MSI_dispatcher(u64 rsp, u64 irqId) {
	SMP_CPUInfoPkg *pkg = SMP_current;
	if (!pkg->intrDesc[irqId - 0x40]) { printk(WHITE, BLACK, "No handler for pci irq %#04x\n", irqId); return ; }
	PCIe_MSI_Descriptor *desc = container(pkg->intrDesc[irqId - 0x40], PCIe_MSI_Descriptor, intrDesc);
	desc->intrDesc.handler(desc->intrDesc.param, (PtReg *)rsp);
	desc->intrDesc.controller->ack(irqId);
}

#define buildIrq(num) Intr_buildIrq(pciIrq, num, HW_PCIe_MSI_dispatcher)

// 0x40 ~ 0x7f
buildIrq(0x40)	buildIrq(0x41)	buildIrq(0x42)	buildIrq(0x43)	buildIrq(0x44)	buildIrq(0x45)	buildIrq(0x46)	buildIrq(0x47)
buildIrq(0x48)	buildIrq(0x49)	buildIrq(0x4a)	buildIrq(0x4b)	buildIrq(0x4c)	buildIrq(0x4d)	buildIrq(0x4e)	buildIrq(0x4f)
buildIrq(0x50)	buildIrq(0x51)	buildIrq(0x52)	buildIrq(0x53)	buildIrq(0x54)	buildIrq(0x55)	buildIrq(0x56)	buildIrq(0x57)
buildIrq(0x58)	buildIrq(0x59)	buildIrq(0x5a)	buildIrq(0x5b)	buildIrq(0x5c)	buildIrq(0x5d)	buildIrq(0x5e)	buildIrq(0x5f)
buildIrq(0x60)	buildIrq(0x61)	buildIrq(0x62)	buildIrq(0x63)	buildIrq(0x64)	buildIrq(0x65)	buildIrq(0x66)	buildIrq(0x67)
buildIrq(0x68)	buildIrq(0x69)	buildIrq(0x6a)	buildIrq(0x6b)	buildIrq(0x6c)	buildIrq(0x6d)	buildIrq(0x6e)	buildIrq(0x6f)
buildIrq(0x70)	buildIrq(0x71)	buildIrq(0x72)	buildIrq(0x73)	buildIrq(0x74)	buildIrq(0x75)	buildIrq(0x76)	buildIrq(0x77)
buildIrq(0x78)	buildIrq(0x79)	buildIrq(0x7a)	buildIrq(0x7b)	buildIrq(0x7c)	buildIrq(0x7d)	buildIrq(0x7e)	buildIrq(0x7f)

void (*HW_PCIe_MSI_intrList[0x40])(void) = {
	pciIrq0x40Interrupt, pciIrq0x41Interrupt, pciIrq0x42Interrupt, pciIrq0x43Interrupt,
	pciIrq0x44Interrupt, pciIrq0x45Interrupt, pciIrq0x46Interrupt, pciIrq0x47Interrupt,
	pciIrq0x48Interrupt, pciIrq0x49Interrupt, pciIrq0x4aInterrupt, pciIrq0x4bInterrupt,
	pciIrq0x4cInterrupt, pciIrq0x4dInterrupt, pciIrq0x4eInterrupt, pciIrq0x4fInterrupt,
	pciIrq0x50Interrupt, pciIrq0x51Interrupt, pciIrq0x52Interrupt, pciIrq0x53Interrupt,
	pciIrq0x54Interrupt, pciIrq0x55Interrupt, pciIrq0x56Interrupt, pciIrq0x57Interrupt,
	pciIrq0x58Interrupt, pciIrq0x59Interrupt, pciIrq0x5aInterrupt, pciIrq0x5bInterrupt,
	pciIrq0x5cInterrupt, pciIrq0x5dInterrupt, pciIrq0x5eInterrupt, pciIrq0x5fInterrupt,
	pciIrq0x60Interrupt, pciIrq0x61Interrupt, pciIrq0x62Interrupt, pciIrq0x63Interrupt,
	pciIrq0x64Interrupt, pciIrq0x65Interrupt, pciIrq0x66Interrupt, pciIrq0x67Interrupt,
	pciIrq0x68Interrupt, pciIrq0x69Interrupt, pciIrq0x6aInterrupt, pciIrq0x6bInterrupt,
	pciIrq0x6cInterrupt, pciIrq0x6dInterrupt, pciIrq0x6eInterrupt, pciIrq0x6fInterrupt,
	pciIrq0x70Interrupt, pciIrq0x71Interrupt, pciIrq0x72Interrupt, pciIrq0x73Interrupt,
	pciIrq0x74Interrupt, pciIrq0x75Interrupt, pciIrq0x76Interrupt, pciIrq0x77Interrupt,
	pciIrq0x78Interrupt, pciIrq0x79Interrupt, pciIrq0x7aInterrupt, pciIrq0x7bInterrupt,
	pciIrq0x7cInterrupt, pciIrq0x7dInterrupt, pciIrq0x7eInterrupt, pciIrq0x7fInterrupt
};

void HW_PCIe_MSI_initDesc(PCIe_MSI_Descriptor *desc, int cpuId, int irqId, IntrHandler handler, u64 param) {
	desc->cpuId = cpuId;
	desc->vec = irqId;
	desc->intrDesc.handler = handler;
	desc->intrDesc.param = param;
	desc->intrDesc.controller = &_MSI_defaultController;
}

void HW_PCIe_MSI_setIntr(PCIe_MSI_Descriptor *desc) {
	SMP_CPUInfoPkg *pkg = SMP_getCPUInfoPkg(desc->cpuId);
	pkg->intrDesc[desc->vec - 0x40] = &desc->intrDesc;
	Intr_Gate_setSMPIntr(desc->cpuId, desc->vec, 0, HW_PCIe_MSI_intrList[desc->vec - 0x40]);
}

void HW_PCIe_MSI_maskIntr(PCIe_MSICapability *cap, int intrId) {
    if (intrId != -1) cap->mask |= (1u << intrId);
    else cap->mask = 0xffffffffu;
}
void HW_PCIe_MSI_unmaskIntr(PCIe_MSICapability *cap, int intrId) {
    if (intrId != -1) cap->mask &= ~(1u << intrId);
    else cap->mask = 0;
}
void HW_PCIe_MSI_setMsgAddr(PCIe_MSICapability *msi, u32 apicId, int redirect, int destMode) {
	msi->msgAddr = 0xfee00000u | (apicId << 12) | (redirect << 3) | (destMode << 2);
}

void HW_PCIe_MSI_setMsgData(PCIe_MSICapability *msi, u32 vec, u32 deliverMode, u32 level, u32 triggerMode) {
	msi->msgData = vec | (deliverMode << 8) | (level << 14) | (triggerMode << 15);
}

void HW_PCIe_MSIX_setMsgAddr(PCIe_MSIX_Table *tbl, int intrId, u32 apicId, int redirect, int destMode) {
	tbl[intrId].msgAddr = 0xfee00000u | (apicId << 12) | (redirect << 3) | (destMode << 2);
}

void HW_PCIe_MSIX_setMsgData(PCIe_MSIX_Table *tbl, int intrId, u32 vec, u32 deliverMode, u32 level, u32 triggerMode) {
	tbl[intrId].msgData = vec | (deliverMode << 8) | (level << 14) | (triggerMode << 15);
}

void HW_PCIe_MSIX_maskIntr(PCIe_MSIX_Table *tbl, int intrId) {
	tbl[intrId].vecCtrl |= 1;
}

void HW_PCIe_MSIX_unmaskIntr(PCIe_MSIX_Table *tbl, int intrId) {
	tbl[intrId].vecCtrl &= ~1u;
}
