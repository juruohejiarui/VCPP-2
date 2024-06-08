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
            u32 bsAddr0, bsAddr1, bsAddr2, bsAddr3, bsAddr4, bsAddr5;
            u32 cardbusCIS;
            u16 subsysID, subsysVendorID;
            u32 expROMBaseAddr;
            u16 reserved0;
            u8 reserved1, capPtr;
            u32 reserved2;
            u8 mxLat, minGnt, intPin, intLine;
        } __attribute__ ((packed)) type0;
        struct {
            u32 bsAddr0, bsAddr1, bsAddr2, bsAddr3;
            u8 capPtr;
            u8 reserved0;
            u16 reserved1;
            u32 reserved2;
            u8 intLine, intPin;
            u16 bridgeCtrl;
        } __attribute__ ((packed)) type1;
    } __attribute__ ((packed)) type;
} __attribute__ ((packed)) PCIDevice;

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

void HW_PCIe_init();

#endif