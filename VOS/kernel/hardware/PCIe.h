#ifndef __HAREWARE_PCIE_H__
#define __HAREWARE_PCIE_H__
#include "../includes/lib.h"
#include "../includes/memory.h"
#include "../includes/interrupt.h"
#include "UEFI.h"
#include "device.h"

typedef struct {
    u16 vendorID, devID;
    u16 command, status;
    u8 revID, progIF, subclass, classCode;
    u8 cacheLineSize, latencyTimer, headerType, BIST;
    union {
        struct {
            u32 bar[6];
            u32 cardbusCISPtr;
            u16 subsysVendorID, subsysID;
            u32 expROMBase;
            u8 capPtr, resv[3];
            u32 resv1;
            u8 intLine, intPin, minGnt, maxLat;
        } __attribute__ ((packed)) type0;
        struct {
            u32 bar[2];
            u8 primaryBus, secBus, subBus, secLatency;
            u8 ioBase, ioLimit;
            u16 secStatus;
            u16 memBase, memLimit;
            u16 preMemBase, preMemLimit;
            u32 preMemBaseUpper, preMemLimitUpper;
            u16 ioBaseUpper, ioLimitUpper;
            u8 capPtr, resv[3];
            u32 expROMBase;
            u8 intLine, intPin, bridgeCtrl;
        } __attribute__ ((packed)) type1;
        struct {
            u32 cardbusBase;
            u8 capPtr, resv;
            u16 secStatus;
            u8 pciBus, cardBus, subBus, cardBusLatency;
            u32 memBase0, memLimit0;
            u32 memBase1, memLimit1;
            u32 ioBase0, ioLimit0;
            u32 ioBase1, ioLimit1;
            u8 intLine, intPin;
            u16 bridgeCtrl;
            u16 subsysDeviceID;
            u16 subsysVendorID;
            u32 legacyBase;
        } __attribute__ ((packed)) type2;
    } __attribute__ ((packed)) type;
} __attribute__ ((packed)) PCIeConfig;

// power status and control registers
typedef struct {
    u16 powerStatus;
    u16 powerCtrl;
} __attribute__ ((packed)) PCIePowerRegs;

typedef struct PCIeManager {
    List listEle;
    PCIeConfig *cfg;
    u8 bus, slot, func;
} PCIeManager; 

typedef struct {
    u64 address;
    u16 segment;
    u8 stBus;
    u8 edBus;
    u32 reserved;
} __attribute__ ((packed)) PCIeStruct;
typedef struct {
	ACPIHeader header;
	u64 reserved;
	PCIeStruct structs[0];
} __attribute__ ((packed)) PCIeDescriptor;

typedef struct PCIe_CapabilityHeader {
	u8 capId;
	u8 nxtPtr;
} __attribute__ ((packed)) PCIe_CapabilityHeader;

typedef struct PCIe_MSICapability {
	PCIe_CapabilityHeader hdr;
	u16 msgCtrl;
	u64 msgAddr;
	u16 msgData;
	u16 reserved;
	u32 mask;
	u32 pending;
} __attribute__ ((packed)) PCIe_MSICapability;

typedef struct PCIe_MSIXCapability {
	PCIe_CapabilityHeader hdr;
	u16 msgCtrl;
	u32 dw1;
	#define PCIe_MSIXCapability_bir(cap) ((cap)->dw1 & 0x7)
	#define PCIe_MSIXCapability_tblOff(cap) ((cap)->dw1 & ~0x7u)
	u32 dw2;
	#define PCIe_MSIXCapability_pendingBir(cap) ((cap)->dw2 & 0x7)
	#define PCIe_MSIXCapability_pendingTblOff(cap) ((cap)->dw2 & ~0x7u)
} __attribute__ ((packed)) PCIe_MSIXCapability;

typedef struct PCIe_MSIX_Table {
	u64 msgAddr;
	u32 msgData;
	u32 vecCtrl;
} __attribute__ ((packed)) PCIe_MSIX_Table;

typedef struct PCIe_MSI_Descriptor {
	int cpuId, vec;
	// the interrupt descriptor to compatible with normal interrupt operation
	IntrDescriptor intrDesc;
} PCIe_MSI_Descriptor;

#define PCIe_CapId_MSI	0x05
#define PCIe_CapId_MSIX	0x11

extern char *PCIeClassDesc[256][256]; 

static __always_inline__ PCIeConfig *HW_PCIe_getDevPtr(u64 addrBase, u8 bus, u8 slot, u8 func) {
    return (PCIeConfig *)DMAS_phys2Virt(addrBase | ((u64)bus << 20) | ((u64)slot << 15) | ((u64)func << 12));
}

static __always_inline__ PCIe_CapabilityHeader *HW_PCIe_getNxtCapHdr(PCIeConfig *cfg, PCIe_CapabilityHeader *hdr) {
	if (hdr) return hdr->nxtPtr ? (PCIe_CapabilityHeader *)((u64)cfg + hdr->nxtPtr) : NULL;
	return (PCIe_CapabilityHeader *)((u64)cfg + cfg->type.type0.capPtr);
}

List *HW_PCIe_getMgrList();
void HW_PCIe_init();

void HW_PCIe_MSI_initDesc(PCIe_MSI_Descriptor *desc, int cpuId, int irqId, IntrHandler handler, u64 param);
void HW_PCIe_MSI_setIntr(PCIe_MSI_Descriptor *desc);
void HW_PCIe_MSI_maskIntr(PCIe_MSICapability *cap, int intrId);
void HW_PCIe_MSI_unmaskIntr(PCIe_MSICapability *cap, int intrId);

void HW_PCIe_MSI_setMsgAddr(PCIe_MSICapability *msi, u32 apicId, int redirect, int destMode);
void HW_PCIe_MSI_setMsgData(PCIe_MSICapability *msi, u32 vec, u32 deliverMode, u32 level, u32 triggerMode);

static __always_inline__ PCIe_MSIX_Table *HW_PCIe_MSIX_getTable(PCIeConfig *cfg, PCIe_MSIXCapability *cap) {
	return DMAS_phys2Virt((*(u64 *)&cfg->type.type0.bar[PCIe_MSIXCapability_bir(cap)] & ~0xf) + PCIe_MSIXCapability_tblOff(cap));
}
void HW_PCIe_MSIX_setMsgAddr(PCIe_MSIX_Table *tbl, int intrId, u32 apicId, int redirect, int destMode);
void HW_PCIe_MSIX_setMsgData(PCIe_MSIX_Table *tbl, int intrId, u32 vec, u32 deliverMode, u32 level, u32 triggerMode);
void HW_PCIe_MSIX_maskIntr(PCIe_MSIX_Table *tbl, int intrId);
void HW_PCIe_MSIX_unmaskIntr(PCIe_MSIX_Table *tbl, int intrId);


#endif