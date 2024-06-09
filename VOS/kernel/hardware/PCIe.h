#ifndef __HAREWARE_PCIE_H__
#define __HAREWARE_PCIE_H__
#include "../includes/lib.h"
#include "UEFI.h"

typedef struct {
    u16 devID;
    u16 vendorID;
    u16 status;
    u16 command;
    u8 classCode, subClass;
    u8 progIF, revID;
    u8 BIST;
    u8 headerType;
    u8 latencyTimer;
    u8 cacheLineSize;
    union {
        struct {
            u32 bar0, bar1, bar2, bar3, bar4, bar5;
            u32 cardbusCIS;
            u16 subsysID, subsysVendorID;
            u32 expROMBaseAddr;
            u16 reserved0;
            u8 reserved1, capPtr;
            u32 reserved2;
            u8 mxLat, minGnt, intPin, intLine;
        } __attribute__ ((packed)) type0;
        struct {
            u32 bar0, bar1;
            u8 secLatencyTimer, subBusNum, secBusNum, PrmyBusNum;
            u16 secStatus;
            u8 ioLimit, ioBase;
            u16 prefMemLimit, prefMemBase;
            u32 prefBase32;
            u32 prefLimit32;
            u16 ioLimit16, IOBase16;
            u32 reserved0;
            u16 reserved1, capPointer;
            u32 expROMBaseAddr;
            u16 bridgeCtrl;
            u8 intrPIN, intrLine;
        } __attribute__ ((packed)) type1;
        struct {
            u32 cardBusBaseAddr;
            u16 secStatus;
            u8 reserved0, offsetCapList;
            u8 cardBusLatencyTimer, subBusNum, cardBusNum, PCIBusNum;
            u32 memBaseAddr0, memLimit0;
            u32 memBaseAddr1, memLimit1;
            u32 ioBaseAddr0, ioLimit0;
            u32 ioBaseAddr1, ioLimit1;
            u16 bridgeCtrl;
            u8 intrPIN, intrLine;
            u16 subsysVendorID, subsysDevID;
            u32 cardBaseAddr16;
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