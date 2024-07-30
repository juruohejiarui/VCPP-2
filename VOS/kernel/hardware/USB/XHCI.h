#ifndef __HAREWARE_USB_XHCI_H__
#define __HAREWARE_USB_XHCI_H__

#include "../PCIe.h"
#include "../../includes/memory.h"
#include "./XHCI/desc.h"

// capability registers
typedef struct {
	u8 capLen;
	u8 reserved;
	u16 version;
	u32 hcsParams1, hcsParams2, hcsParams3;
	u32 hccParams1;
	u32 dboff, rtsoff;
	u32 hccParams2;
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
	volatile u32 cmd;
	volatile u32 doorbell[0];
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

#include "./XHCI/ctx.h"

struct USB_XHCIController;
struct USB_XHCI_ReqBlock;

// device context data block
typedef struct {
	USB_XHCI_DeviceSlotContext slot;
	USB_XHCI_EndpointContext ep[0];
} __attribute__ ((packed)) USB_XHCI_DeviceContext;

#include "./XHCI/trb.h"

typedef struct {
	USB_XHCI_InputCtrlContext inCtx;
	USB_XHCI_DeviceSlotContext slotCtx;
	USB_XHCI_EndpointContext epCtx[31];
} __attribute__ ((packed)) USB_XHCI_InputContext;

typedef struct {
	// pointer to the read-only context in controller
	USB_XHCI_DeviceContext *roctx;
	// one bit map represents the state of one endpoint
	u32 enableEp;
	// zero based slot index
	u32 slot;

	// pointers to the transfer rings
	USB_XHCI_GenerTRB *transRing[31];
	// the inqueue pointers of each transfer rings
	USB_XHCI_GenerTRB *transInqPtr[31];
	// the source of request of each transfer TRB
	struct USB_XHCI_ReqBlock **transSrc[31];
	// the cycle flags of each transfer rings
	u8 transCycFlags[31];
	
	// the controller that this device belongs to.
	struct USB_XHCIController *ctrl;

	// the copy of teach contexts
	USB_XHCI_InputContext *ctx;

	// descriptors
	USB_XHCI_DevDesc *desc;
	USB_XHCI_ConfigDesc **cfgDesc;

	u8 *strDesc;
	
} USB_XHCI_Device;

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

#define HW_USB_XHCIReq_Flag_failed			(1 << 0)
#define HW_USB_XHCIReq_Flag_isCommand		(1 << 1)
#define HW_USB_XHCIReq_Flag_replied			(1 << 2)

typedef void (*USB_XHCI_ReqAck)(struct USB_XHCIController *, struct USB_XHCI_ReqBlock *, void *);

typedef struct USB_XHCI_ReqBlock {
	// there are at most 256 requst in one reqBlocks
	int reqCnt;
	int slot, endpoint;

	u8 flags;

	List listEle;

	void *arg;
	USB_XHCI_ReqAck ack;

	USB_XHCI_GenerTRB res, reqs[0];
} __attribute__((packed)) USB_XHCI_ReqBlock;

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

	// device context, the address here is physical address
	USB_XHCI_DeviceContext **devCtx;
	USB_XHCI_Device **devices;

	USB_XHCI_EveRingSegTblEntry **eveRingSegTbls;
	USB_XHCI_RingFlag *eveRingFlag;

	USB_XHCI_GenerTRB *cmdRing;
	USB_XHCI_RingFlag cmdRingFlag;
	// the flags of each command in the command ring
	// where the command in the command ring is from
	USB_XHCI_ReqBlock **cmdSrc;

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

u64 HW_USB_XHCI_mainThread(u64 (*_)(u64), u64 ctrlAddr);

u64 HW_USB_XHCI_devThread(u64 (*_)(u64), u64 devAddr);


#define HW_USB_XHCI_DriverCheck_Unmatched		0
#define HW_USB_XHCI_DriverCheck_PartlySuccess	1
#define HW_USB_XHCI_DriverCheck_Success			2

typedef struct USB_XHCI_Driver {
	u64 (*loader)(USB_XHCI_Device *dev);
	int (*chk)(USB_XHCI_Device *dev);
	char *name;
	List listEle;
} USB_XHCI_Driver;

extern List HW_USB_XHCI_drvList;
extern SpinLock HW_USB_XHCI_drvListLock;

void HW_USB_XHCI_insReqBlk(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *reqs);

int HW_USB_XHCI_waitRely(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *reqs);

int HW_USB_XHCI_chkSucc(USB_XHCI_ReqBlock *reqs);

static void HW_USB_XHCI_normalAck(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev);

USB_XHCI_ReqBlock *HW_USB_XHCI_mkCmdBlk(int trbType, u64 slot, u64 arg);

USB_XHCI_ReqBlock *HW_USB_XHCI_mkGetDescBlk(u64 slot, u64 descType, u64 idx, u64 wIdx, u64 len, void *buf);

USB_XHCI_ReqBlock *HW_USB_XHCI_mkSetCfgBlk(u64 slot, u64 cfgVal);

USB_XHCI_ReqBlock *HW_USB_XHCI_mkGetDataBlk(u64 slot, u64 epId, u64 len, void *buf);

USB_XHCI_ReqBlock *HW_USB_XHCI_mkSetDataBlk(u64 slot, u64 epId, u64 len, void *buf);

void HW_USB_XHCI_freeReqBlk(USB_XHCI_ReqBlock *reqs);

void HW_USB_XHCI_addDriver(USB_XHCI_Driver *drv);

void HW_USB_XHCI_delDriver(USB_XHCI_Driver *drv);

USB_XHCI_Driver *HW_USB_XHCI_getDriver(USB_XHCI_Device *dev);

USB_XHCI_DescHeader *HW_USB_XHCI_getNxtDesc(USB_XHCI_ConfigDesc *cfg, USB_XHCI_DescHeader *hdr);

#endif