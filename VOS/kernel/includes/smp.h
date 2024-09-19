#ifndef __SMP_H__
#define __SMP_H__

#include "lib.h"
#include "hardware.h"
#include "task.h"

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

#define SMP_CPUInfo_flag_InTaskLoop (1ul << 0)
#define SMP_CPUInfo_flag_APUInited	(1ul << 1)
#define SMP_CPUInfo_flag_WaitTask	(1ul << 2)

typedef struct SMP_CPUInfoPkg {
	// the topo index of this processor
	u32 apicID;
	u32 trIdx;
	u64 *initStk;
	TaskStruct *simdRegDomain;
	u32 *tssTable;
	u16 reserved;
	u16 idtTblSize;
	IDTItem *idtTable;
	u64 intrMsk[4];
	IntrDescriptor *intrDesc[0x40];
	volatile u64 flags;
	void *ipiMsg;
} __attribute__ ((packed)) SMP_CPUInfoPkg;

#define SMP_IPI_Type_Schedule	0xc8

IntrHandlerDeclare(SMP_irq0xc8Handler);

extern u8 SMP_APUBootStart[];
extern u8 SMP_APUBootEnd[];

extern SMP_CPUInfoPkg SMP_cpuInfo[Hardware_CPUNumber];
extern int SMP_cpuNum;
extern Atomic SMP_task0LaunchNum;

#define SMP_current (SMP_getCPUInfoPkg(SMP_task0LaunchNum.value == SMP_cpuNum ? Task_current->cpuId : SMP_getCurCPUIndex()))

void SMP_init();

void SMP_sendIPI(int cpuId, u32 vector, void *msg);
void SMP_sendIPI_all(u32 vector, void *msg);
void SMP_sendIPI_self(u32 vector, void *msg);
void SMP_sendIPI_allButSelf(u32 vector, void *msg);

void startSMP();

u32 SMP_getCurCPUIndex();

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx);

void SMP_maskIntr(int cpuId, u8 vecSt, u8 vecNum);

/// @brief test whether the vector in range [vecSt, vecSt + vecNum - 1] are all unmasked
/// @return -1: if all the vectors are unmasked, return the first maksed vector otherwise.
int SMP_testIntr(int cpuId, u8 vecSt, u8 vecNum);

// Select a CPU and assign consecutive NUM interrupt vectors
// if there is not enough vector, then this allocation will be canceled and *cpuId == -1
void SMP_allocIntrVec(int num, int *cpuId, u8 *vecSt);
#endif