#ifndef __HAREWARE_USB_XHCI_H__
#define __HAREWARE_USB_XHCI_H__

#include "../PCIe.h"
#include "../../includes/memory.h"

// capability registers
typedef struct {
	u8 capLen;
	u8 reserved;
	u16 version;
	u32 hcsParams1, hcsParams2, hcsParams3;
	u32 hccparam1;
	u32 dboff, rtsoff;
	u32 hccparam2;
} __attribute__ ((packed)) USB_XHCI_CapRegs;

// port registers
typedef struct {
	u32 statusCtrl;
	u32 pwStatusCtrl;
	u32 linkInfo;
	u32 reserved;
} __attribute__ ((packed)) USB_XHCI_PortRegs;

#define HW_USB_XHCI_Port_StatusCtrl_connectStatus(port) 		((port)->statusCtrl & 0x1)
#define HW_USB_XHCI_Port_StatusCtrl_portEnable(port) 			(((port)->statusCtrl >> 1) & 0x1)
#define HW_USB_XHCI_Port_StatusCtrl_portReset(port) 			(((port)->statusCtrl >> 4) & 0x1)
#define HW_USB_XHCI_Port_StatusCtrl_portLinkState(port) 		(((port)->statusCtrl >> 5) & 0xf)
#define HW_USB_XHCI_Port_StatusCtrl_portSpeed(port) 			(((port)->statusCtrl >> 10) & 0xf)

#define HW_USB_XHCI_OpReg_Cmd_HCReset	1

#define HW_USB_XHCI_OpReg_Status_HCHalted 		0
#define HW_USB_XHCI_OpReg_Status_CtrlNotReady	11


// operational registers
typedef struct {
	volatile u32 usbCmd, usbStatus;
	u32 pageSize;
	u64 reserved;
	volatile u32 devNotifCtrl;
	volatile u64 cmdRingCtrl;
	u8 reserved1[0x30 - 0x20];
	// the base address of device context array, which is 64-byte aligned
	volatile u64 devCtxBaseAddr;
	u32 config;
	u8 reserved2[0x400 - 0x3c];
	USB_XHCI_PortRegs portRegs[0];
} __attribute__ ((packed)) USB_XHCI_OpRegs;

// interrupt register set
typedef struct {
	// management register
	volatile u32 mgrRegs;
	// interrupt moderation
	u32 mod;
	// event ring segment table size
	u32 eveSegTblSize;
	u32 reserved;
	// event ring segment table base address, which is 64-byte aligned
	u64 eveSegTblAddr;
	// event ring dequeue pointer
	u64 eveDeqPtr;
} __attribute__ ((packed)) USB_XHCI_IntrRegs;

// runtime registers
typedef struct {
	volatile u32 microFrameIndex;
	u8 reserved[28];
	USB_XHCI_IntrRegs intrRegs[0];
} __attribute__ ((packed)) USB_XHCI_RuntimeRegs;

// doorbell registers
typedef struct {
	u32 cmd;
	u32 doorbell[0];
} __attribute__ ((packed)) USB_XHCI_DoorbellRegs;

typedef struct {
	u8 id;
	u8 nxtOff;
	u8 idSpecific[0];
} __attribute__ ((packed)) USB_XHCI_ExtCapEntry;

typedef struct {
	USB_XHCI_ExtCapEntry extCap;
	// bit0 : BIOS Owned Semaphore
	// bit8 : System Software Owned Semaphore
	// other bits are reserved and preserved
	volatile u16 data1;
	// used by BIOS
	u32 legacyCap;
} __attribute__ ((packed)) USB_XHCI_ExtCap_Legacy;

typedef struct {
	USB_XHCI_ExtCapEntry extCap;
	u8 minorRev, majorRev;
	u8 name[4];
	// the compatible port offset
	u8 portOff;
	// the compatible port count
	u8 portCnt;
	u8 protocolDefined, speedIdCnt;
	u8 slotType : 5;
	u32 reserved : 27;
} __attribute__ ((packed)) USB_XHCI_ExtCap_Protocol;

#define HW_USB_XHCI_Port_Flag_Paired	0x1
#define HW_USB_XHCI_Port_Flag_USB3		0x2
#define HW_USB_XHCI_Port_Flag_Master	0x4

#define HW_USB_XHCI_Port_isPaired(port)		((port)->flags & HW_USB_XHCI_Port_Flag_Paired)
#define HW_USB_XHCI_Port_isUSB3(port)		((port)->flags & HW_USB_XHCI_Port_Flag_USB3)
#define HW_USB_XHCI_Port_isMaster(port)		((port)->flags & HW_USB_XHCI_Port_Flag_Master)

#define HW_USB_XHCI_Port_getPair(port)		((USB_XHCI_Port *)((u64)(port) + 1))

typedef struct USB_XHCI_Port {
	USB_XHCI_PortRegs *regs;
	u64 flags, offset;
	u8 slotType;
	struct USB_XHCI_Port *pair;
	Device *dev;
} USB_XHCI_Port;

#pragma region Context Block
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

// device context data block
typedef struct {
	USB_XHCI_DeviceSlotContext slot;
	USB_XHCI_EndpointContext ctx[0];
} __attribute__ ((packed)) USB_XHCI_DeviceContext;

#include "./XHCI/trb.h"

// event ring segment table entry
typedef struct {
	u64 addr;
	u16 size;
	u16 reserved;
	u32 reserved1;
} __attribute__ ((packed)) USB_XHCI_EveRingSegTblEntry;

// event ring segment flag
typedef struct {
	u32 segId;
	u32 pos;
	u8 cycleBit;
} USB_XHCI_RingFlag;

#define HW_USB_XHCI_EveRingSegTblSize	2

#define HW_USB_XHCI_RingEntryNum	(Page_4KSize * 16 / sizeof(USB_XHCI_GenerTRB))

typedef struct {
	u64 addr;
	List listEle;
} __attribute__ ((packed)) USB_XHCI_MemUsage;

#define USB_XHCI_Req_Type_Cmd	1

#define HW_USB_XHCIReq_Flag_Completed		(1 << 0)
#define HW_USB_XHCIReq_Flag_isInRing		(1 << 2)
#define HW_USB_XHCIReq_Flag_isCommand		(1 << 1)

struct USB_XHCIController;

typedef struct USB_XHCIReq {
	USB_XHCI_GenerTRB req, eve;
	u8 flag;
	void *arg;
	void (*handler)(struct USB_XHCIController *ctrl, struct USB_XHCIReq *req, void *arg);
	List listEle;
} USB_XHCIReq;

typedef struct USB_XHCIController {
	Device dev;
	PCIeConfig *config;
	PCIePowerRegs *pwRegs;
	USB_XHCI_CapRegs *capRegs;
	USB_XHCI_OpRegs *opRegs;
	USB_XHCI_RuntimeRegs *rtRegs;
	USB_XHCI_DoorbellRegs *dbRegs;
	USB_XHCI_ExtCapEntry *extCapHeader;
	List listEle, memList;
	USB_XHCI_Port *ports;

	USB_XHCI_GenerTRB *cmdRing;
	USB_XHCI_DeviceContext **devCtx;
	USB_XHCI_EveRingSegTblEntry **eveRingSegTbls;
	USB_XHCI_RingFlag *eveRingFlag;
	USB_XHCI_RingFlag cmdRingFlag;
	// the flags of each command in the command ring
	u64 *cmdsFlag;
	// where the command in the command ring is from
	USB_XHCIReq **cmdSrc;

	List witReqList;

	SpinLock lock, witQueLock;
} USB_XHCIController;

#define USB_XHCI_ExtCap_Id_Legacy 	0x01
#define USB_XHCI_ExtCap_Id_Protocol 0x02
#define USB_XHCI_ExtCap_Id_Power 	0x03
#define USB_XHCI_ExtCap_Id_IOVirt	0x04
#define USB_XHCI_ExtCap_Id_Message 	0x05
#define USB_XHCI_ExtCap_Id_LocalMem 0x06
#define USB_XHCI_ExtCap_Id_Debug	0x0a
#define USB_XHCI_ExtCap_Id_MsgIntr	0x11

extern List HW_USB_XHCI_mgrList;

/// @brief initialize the xhci controller
/// @param xhci the xhci structure in PCIe
/// @return 1: initialzation success, 0: initialization failed
int HW_USB_XHCI_Init(PCIeConfig *xhci);

u64 HW_USB_XHCI_thread(u64 (*_)(u64), u64 ctrlAddr);

#endif