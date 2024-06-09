#ifndef __HAREWARE_PCIE_H__
#define __HAREWARE_PCIE_H__
#include "../includes/lib.h"
#include "UEFI.h"

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
} __attribute__ ((packed)) PCIeDevice;

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

extern char *PCIeClassDesc[256][256];

void HW_PCIe_init();

#endif