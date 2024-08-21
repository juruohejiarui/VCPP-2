#ifndef __HARDWARE_APIC_H__
#define __HARDWARE_APIC_H__
#include "../includes/lib.h"

typedef struct {
	u32 vector 			: 8; // 0-7
	u32 deliveryMode 	: 3; // 8-10
	u32 destMode 		: 1; // 11
	u32 deliveryStatus 	: 1; // 12
	u32 pinPolarity 	: 1; // 13
	u32 remoteIRR 		: 1; // 14
	u32 triggerMode 	: 1; // 15
	u32 mask 			: 1; // 16
	u32 reserved 		: 15;// 17-31

	union {
		struct {
			u32 reserved1 	: 24; // 0-23
			u32 destination : 8;  // 24-31
		} __attribute__ ((packed)) logical;

		struct {
			u32 reserved2 	: 24; // 0-23
			u32 destination : 4;  // 24-27
			u32 reserved3 	: 4;  // 28-31
		} __attribute__ ((packed)) physical;
	} destDesc;

} __attribute__ ((packed)) APICRteDescriptor;

typedef struct APIC_ICRDescriptor {
	u32 vector 			: 8;
	u32 deliverMode 	: 3;
	u32 destMode 		: 1;
	u32 deliverStatus	: 1;
	u32 reserved1		: 1;
	u32 level			: 1;
	u32 triggerMode		: 1;
	u32 reserved2		: 2;
	u32 DestShorthand	: 2;
	u32 reserved3		: 12;
	union {
		struct {
			u32 reserved : 24;
			u8 dest : 8;
		} __attribute__ ((packed)) apic;
		u32 x2Apic;
	} __attribute__ ((packed)) dest;
} __attribute__ ((packed)) APIC_ICRDescriptor;

// delivery mode
#define HW_APIC_DeliveryMode_Fixed 			0x0
#define HW_APIC_DeliveryMode_LowestPriority 0x1
#define HW_APIC_DeliveryMode_SMI 			0x2
#define HW_APIC_DeliveryMode_NMI 			0x4
#define HW_APIC_DeliveryMode_INIT 			0x5
#define HW_APIC_DeliveryMode_Startup 		0x6
#define HW_APIC_DeliveryMode_ExtINT 		0x7

// mask
#define HW_APIC_Mask_Unmasked 	0x0
#define HW_APIC_Mask_Masked 	0x1

// trigger mode
#define HW_APIC_TriggerMode_Edge 	0x0
#define HW_APIC_TriggerMode_Level 	0x1

// delivery status
#define HW_APIC_DeliveryStatus_Idle 	0x0
#define HW_APIC_DeliveryStatus_Pending 	0x1

// destination shorthand
#define HW_APIC_DestShorthand_None 			0x0
#define HW_APIC_DestShorthand_Self 			0x1
#define HW_APIC_DestShorthand_AllIncludingSelf 0x2
#define HW_APIC_DestShorthand_AllExcludingSelf 0x3

// destination mode
#define HW_APIC_DestMode_Physical 	0x0
#define HW_APIC_DestMode_Logical 	0x1

// level
#define HW_APIC_Level_Deassert	0x0
#define HW_APIC_Level_Assert 	0x1

// remote IRR
#define HW_APIC_RemoteIRR_Reset 	0x0
#define HW_APIC_RemoteIRR_Accept 	0x1

// pin polarity
#define HW_APIC_PinPolarity_High 0x0
#define HW_APIC_PinPolarity_Low  0x1


u64 Hardware_APIC_getReg_IA32_APIC_BASE();
void HW_APIC_setReg_IA32_APIC_BASE(u64 value);

void HW_APIC_writeRTE(u8 index, u64 val);

void HW_APIC_suspend();
void HW_APIC_resume();

void HW_APIC_disableIntr(u8 intrId);
void HW_APIC_enableIntr(u8 intrId);

void HW_APIC_install(u8 intrId, void *arg);
void HW_APIC_uninstall(u8 intrId);

void HW_APIC_edgeAck(u8 irqId);

void HW_APIC_init();

int HW_APIC_finishedInit();
#endif