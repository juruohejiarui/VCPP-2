#ifndef __SMP_H__
#define __SMP_H__

#include "lib.h"
#include "hardware.h"

typedef struct MADTDescriptor {
	ACPIHeader header;
	u32 localAPICAddr;
	u32 flags;
	struct MADTEntry {
		u8 type;
		u8 length;
		union MADTEntryContent{
			// processor local apic
			struct MADTEntry_Type0 {
				u8 processorID;
				u8 apicID;
				u32 flags;
			} __attribute__ ((packed)) type0;
			// ioapic
			struct MADTEntry_Type1 {
				u8 ioapicID;
				u8 reserved;
				u32 ioapicAddr;
				u32 gloSysIntrBase;
			} __attribute__ ((packed)) type1;
			// ioapic interrupt source override
			struct MADTEntry_Type2 {
				u8 busSrc;
				u8 irqSrc;
				u32 gloSysIntr;
				u16 flags;
			} __attribute__ ((packed)) type2;
			// ioapic non-maskable interrupt source
			struct MADTEntry_Type3 {
				u8 nmiSrc;
				u8 reserved;
				u16 flags;
				u32 gloSysIntrBase;
			} __attribute__ ((packed)) type3;
			// local apic non-maskable interrupts
			struct MADTEntry_Type4 {
				u8 processorID; // 0xff means all processors
				u16 flags;
				u8 lint;
			} __attribute__ ((packed)) type4;
			// local apic address override
			struct MADTEntry_Type5 {
				u16 reserved;
				u64 addr;
			} __attribute__ ((packed)) type5;
			// processor local x2apic
			struct MADTEntry_Type9 {
				u16 reserved;
				u32 x2apicID;
				u32 flags;
				u32 apicID;
			} __attribute__ ((packed)) type9;
		} __attribute__ ((packed)) ct;
	} __attribute__ ((packed)) entries[0];
} MADTDescriptor;

#define SMP_CPUInfo_flag_InTaskLoop (1 << 0)
#define SMP_CPUINfo_flag_APUInited	(1 << 1)

typedef struct SMP_CPUInfoPkg {
	u32 cpuId;
	u32 trIdx;
	u64 *initStk;
	struct TaskStruct *simdRegDomain;
	u32 *tssTable;
	u64 flags;
} __attribute__ ((packed)) SMP_CPUInfoPkg;

#define SMP_current (SMP_getCPUInfoPkg(SMP_getCurCPUIndex()))

extern u8 SMP_APUBootStart[];
extern u8 SMP_APUBootEnd[];

extern SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];


void SMP_init();

u32 SMP_getCurCPUIndex();

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx);
#endif