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
	u8 signature[4];  	// 0-3
	u32 length;			// 4-7
	u8 revision;		// 8
	u8 checksum;		// 9
	u8 OEMID[6];		// 10-15
	u64 OEMTableID;		// 16-23
	u32 OEMRevision;	// 24-27
	u32 creatorID;		// 28-31
	u32 creatorRevision;// 32-35

	u8 hardwareRevID;			// 36
	u8 comparatorCount : 5;		// 37-41
	u8 counterSize : 1;			// 42
	u8 reserved : 1;			// 43
	u8 legacyReplacement : 1; 	// 44
	u16 pciVendorID;			// 45-46
	AddressStructure address; 	// 47-63
	u32 hpetNumber;				// 64-67
	u16 minimumTick;			// 68-69
	u8 pageProtection;			// 70
} __attribute__ ((packed)) HPETDescriptor;

#define HW_Timer_HPET_Signature "HPET"

void HW_Timer_HPET_init();
i64 HW_Timer_HPET_jiffies();

#endif