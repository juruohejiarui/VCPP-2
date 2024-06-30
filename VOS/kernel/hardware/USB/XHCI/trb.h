#ifndef __HW_USB_XHCI_TRB_H__
#define __HW_USB_XHCI_TRB_H__

#include "../../../includes/lib.h"

#pragma region transfer request block (TRB)

#define HW_USB_TrbType_Link				6
#define HW_USB_TrbType_NoOp				8
#define HW_USB_TrbType_EnblSlotCmd		9
#define HW_USB_TrbType_NoOpCmd			23
#define HW_USB_TrbType_CmdCompletionEve	33

// general format of transfer request block
typedef struct {
	u32 dw[3];
	// the fourth dword
	union {
		struct {
			u8 cycle : 1;
			u16 reserved : 9;
			u8 trbType : 6;
			u16 reserved1;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw3;
} __attribute__ ((packed)) USB_XHCI_GenerTRB;

// normal transfer request block
typedef struct {
	// first dword and second dword
	union {
		// data buffer pointer or immediate data
		u64 dtBufPtr;
		struct {
			u32 dw1, dw2;
		} __attribute__ ((packed)) raw;
	} __attribute__ ((packed)) dw0_1;
	// third dword
	union {
		struct {
			// TRB transfer length
			u32 trbLen : 17;
			// Transfer data size
			u8 tdSize : 5;
			// interrupter target, the interrupter to which the device sends its interrupts
			u16 intrIrg : 10;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw2;
	// fourth dword
	union {
		struct {
			// cycle bit, 0: normal, 1: cycle
			u8 cycle : 1;
			// reserved
			u8 reserved : 4;
			// interrupt on completion, 0: no interrupt, 1: interrupt
			u8 ioc : 1;
			// immediate data, 0: no immediate data, 1: immediate data
			u8 idt : 1;
			// reserved and zero'd
			u8 reserved1 : 3;
			// TRB type
			u8 trbType : 6;
			// transfer type, 0: no data state, 1: reserved, 2: out data state, 3: in data state
			u8 tfType : 2;
			// reserved and zero'd
			u16 reserved2 : 14;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw3;
} __attribute__ ((packed)) USB_XHCI_NormalTRB;

// setup stage transfer request block
typedef struct {
	// first dword
	union {
		struct {
			u8 bmReqType;
			u8 bReq;
			u16 wVal;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw0;
	// second dword
	union {
		struct {
			u16 wIndex;
			u16 wLen;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw1;
	// third dword
	union {
		struct {
			// TRB Trnasfer Length, should always be 8
			u32 trbLen : 17;
			u8 reserved : 5;
			u32 intrTrg : 10;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw2;
	// fourth dword
	union {
		struct {
			// cycle bit, 0: normal, 1: cycle
			u8 cycle : 1;
			// reserved
			u8 reserved : 4;
			// interrupt on completion, 0: no interrupt, 1: interrupt
			u8 ioc : 1;
			// immediate data, 0: no immediate data, 1: immediate data
			u8 idt : 1;
			// reserved and zero'd
			u8 reserved1 : 3;
			// TRB type
			u8 trbType : 6;
			// transfer type, 0: no data state, 1: reserved, 2: out data state, 3: in data state
			u8 tfType : 2;
			// reserved and zero'd
			u16 reserved2 : 14;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw3;
} __attribute__ ((packed)) USB_XHCI_setupTRB;

// data stage transfer request block
typedef struct {
	union {
		u64 dtBuf;
		struct {
			u32 dw0;
			u32 dw1;
		} __attribute__ ((packed)) raw;
	} __attribute__ ((packed)) dw0_1;
	// third dword
	union {
		struct {
			// TRB transfer length
			u32 trbLen : 17;
			u8 tdSize : 5;
			u16 intrTrg : 10;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw2;
	// fourch dword
	union {
		struct {
			u8 cycle : 1;
			u8 evalNxtTRB : 1;
			u8 shortPktIntr : 1;
			u8 noSnoop : 1;
			u8 chainBit : 1;
			// interrupt on completion
			u8 ioc : 1;
			// immediate data
			u8 idt : 1;
			u8 reserved : 3;
			// TRB Type
			u8 trbType: 6;
			u8 direct : 1;
			u16 reserved1 : 15;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} dw3;
} __attribute__ ((packed)) USB_XHCI_dataTRB;

// status state transfer request block
typedef struct {
	u64 reserved; // the first and second dwords are reserved
	// the third dword
	union {
		struct {
			u32 reserved : 22;
			u16 intrTrg : 10;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw2;
	union {
		struct {
			u8 cycle : 1;
			u8 evalNxtTRB : 1;
			u8 reserved : 2;
			u8 chainBit : 1;
			// interrupt on completion
			u8 ioc : 1;
			u8 reserved1 : 4;
			// TRB Type
			u8 trbType : 6;
			u8 direct : 1;
			u16 reserved2 : 15;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw3;
} __attribute__ ((packed)) USB_XHCI_statusTRB;

// command completion transfer request block
typedef struct {
	// first and second dword
	union {
		u64 cmdPtr;
		u8 reserved : 4;
		union {
			u32 dw0;
			u32 dw1;
		} __attribute__ ((packed)) raw;
	} __attribute__ ((packed)) dw0_1;
	// third dword
	union {
		u32 raw;
		struct {
			u8 reserved[3];
			// completion code
			u8 code;
		} __attribute__ ((packed)) ctx;
	} __attribute__ ((packed)) dw2;
	// fourth
	union {
		u32 raw;
		struct {
			u8 cycle : 1;
			u16 reserved : 9;
			u8 trbType : 6;
			u8 virtFuncId;
			u8 slotId;
		} __attribute__ ((packed)) ctx;
	} __attribute__ ((packed)) dw3;
} __attribute__ ((packed)) USB_XHCI_CompletionTRB;
#pragma endregion

#endif