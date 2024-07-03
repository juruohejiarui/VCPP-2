#ifndef __HW_USB_XHCI_CTX_H__
#define __HW_USB_XHCI_CTX_H__

#include "../../../includes/lib.h"

#pragma region Context Block
// input control context
typedef struct {
    u32 dropFlags;
    u32 addFlags;
    u32 reserved[5];
    // config value
    u8 cfgVal;
    // interface number
    u8 intfNum;
    // altermate setting
    u8 alterSet;
    u8 reserved1;
} __attribute__ ((packed)) USB_XHCI_InputCtrlContext;


// slot context
typedef struct {
	// first dword
	union {
		struct {
			// route string
			u32 routeStr : 20;
			// speed, 0: reserved, 1: full-speed, 2: low-speed, 3: high-speed, 4: super-speed
			u8 speed : 4;
			u8 reserved : 1;
			// multi-tt, used by high-speed hub
			u8 multi : 1;
			// hub, set by system software if the device attached is a hub
			u8 hub : 1;
			// the count of valid endpoint contexts structures follows the slot context
			u8 ctxEntries : 5;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw0;
	

	// second dword
	union {
		struct {
			// max exit latency
			u16 maxExitLatency;
			// root hub port number
			u8 rootHubPort;
			// number of downstream ports
			u8 numPorts;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw1;

	// third dword
	union {
		struct {
			// TT hub slot id, used if this device is full-speed or low-speed, and is attached to a full-speed hub
			u8 ttHubSlotId;
			// TT port number, used if this device is full-speed or low-speed, and is attached to a full-speed hub
			u8 ttPortNum;
			// TT Think Time, used if this device is full-speed or low-speed, and is attached to a full-speed hub
			// 0: 8 bit times, 1: 16 bit times, 2: 24 bit times, 3: 32 bit times
			u8 ttThinkTime : 2;
			// reserved
			u8 reserved1 : 4;
			// interrupter target, the interrupter to which the device sends its interrupts
			u16 intTarget : 10;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw2;

	// fourth dword
	union {
		struct {
			// the number of context entries that follow this slot context
			u8 devAddr;
			// reserved
			u64 reserved2 : 19;
			// slot state, 0: disabled, 1: default, 2: addressed, 3: configured 4~31: reserved
			u8 slotState : 5;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw3;

	// used by controller
	u32 reservedOpaque[4];
} __attribute__ ((packed)) USB_XHCI_DeviceSlotContext;

// endpoint context
typedef struct {
	// first dword
	union {
		struct {
			// endpoint state, 0: disabled, 1: running, 2: halted, 3: stopped, 4: error 5~7: reserved
			u8 epState : 3;
			// reserved
			u8 reserved : 4;
			// mult, the number of transactions per microframe
			u8 multi : 2;
			// max primary streams, the number of primary stream IDs that the endpoint supports
			u8 mxPStreams : 5;
			// linear stream array, 0: disabled, 1: enabled
			u8 lsa : 1;
			// interval, the interval for polling endpoint for data transfersq
			u8 interval : 8;
			// max endpoint service time interval payload (high 8 bit)
			u8 mxESITPayloadHi : 8;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw0;
	
	// second dword
	union {
		struct {
			// second dword
		// reserved
		u8 reserved1 : 1;
		// error count, the number of consecutive errors that the endpoint has detected
		u8 errCnt : 2;
		// endpoint type
		// 0 : not valid, 1: Isochoronous OUT, 2: Bulk OUT, 3: Interrupt OUT, 5: Isochoronous IN, 6: Bulk IN, 7: Interrupt IN
		u8 epType : 3;
		// reserved
		u8 reserved2 : 1;
		// host initiate disable, 0: enabled, 1: disabled
		u8 hid : 1;
		// max burst size, the maximum number of packets that the endpoint can send or receive in a burst
		u8 mxBurstSize;
		// max packet size, the maximum packet size that the endpoint can send or receive
		u16 mxPktSize;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw1;

	// third dword & fourth dword
	union {
		u64 trDeqPtr;
		struct {
			u32 trDeqPtrLo;
			u32 trDeqPtrHi;
		} __attribute__ ((packed)) trDeqPtr32;
		u8 deqCycSts : 1;
	} __attribute__ ((packed)) dw2_3;

	// fifth dword
	union {
		struct {
			// average TRB length, the average length of the TRBs that the endpoint sends or receives
			u16 avgTRBLen;
			// max endpoint service time interval payload (low 16 bit)
			u16 mxESITPayloadLo;
		} __attribute__ ((packed)) ctx;
		u32 raw;
	} __attribute__ ((packed)) dw4;

	// used by controller
	u32 reserved[3];
} __attribute__ ((packed)) USB_XHCI_EndpointContext;
#pragma endregion

#endif