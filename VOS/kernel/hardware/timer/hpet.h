#ifndef __HARDWARE_TIMER_HPET_H__
#define __HARDWARE_TIMER_HPET_H__

#include "../../includes/lib.h"

typedef struct {
	u8 addrSpaceId;
	u8 regBitWidth;
	u8 regBitOffset;
	u8 reserved;
	u64 Address;
} __attribute__ ((packed)) AddressStructure;
typedef struct {
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	u8 OEMID[6];
	u64 OEMTableID;
	u32 OEMRevision;
	u32 creatorID;
	u32 creatorRevision;

	u8 hardwareRevID;
	u8 comparatorCount : 5;
	u8 counterSize : 1;
	u8 reserved : 1;
	u8 legacyReplacement : 1;
	u16 pciVendorID;
	AddressStructure address;
	u32 hpetNumber;
	u16 minimumTick;
	u8 pageProtection;
} __attribute__ ((packed)) HPETDescriptor;

#define HW_Timer_HPET_Signature "HPET"

void HW_Timer_HPET_init();

#endif