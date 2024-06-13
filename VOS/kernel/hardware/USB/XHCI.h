#ifndef __HAREWARE_USB_XHCI_H__
#define __HAREWARE_USB_XHCI_H__

#include "../PCIe.h"

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
	u32 usbCmd, usbStatus;
	u32 pageSize;
	u64 reserved;
	u32 devNotifCtrl;
	u64 cmdRingCtrl;
	u8 reserved1[0x30 - 0x20];
	u64 devCtxBaseAddr;
	u32 config;
	u8 reserved2[0x400 - 0x3c];
	USB_XHCI_PortRegs portRegs[0];
} __attribute__ ((packed)) USB_XHCI_OpRegs;

// interrupt register set
typedef struct {
	// management register
	u32 mgrRegs;
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
	u32 microFrameIndex;
	u8 reserved[28];
	USB_XHCI_IntrRegs intrRegs[0];
} __attribute__ ((packed)) USB_XHCI_RuntimeRegs;

// doorbell registers
typedef struct {
	u32 cmd;
	u32 doorbell[0];
} __attribute__ ((packed)) USB_XHCI_DoorbellRegs;

typedef struct {
	Device dev;
	PCIeConfig *config;
	PCIePowerRegs *pwRegs;
	USB_XHCI_CapRegs *capRegs;
	USB_XHCI_OpRegs *opRegs;
	USB_XHCI_RuntimeRegs *rtRegs;
	USB_XHCI_DoorbellRegs *dbRegs;
	List listEle, pageList;
} USB_XHCIController;

typedef struct {
	u8 id;
	u8 nxtOff;
	u8 idSpecific[0];
} __attribute__ ((packed)) USB_XHCI_ExtCapEntry;

#define USB_XHCI_ExtCap_Id_Legacy 	0x01
#define USB_XHCI_ExpCap_Id_Protocol 0x02
#define USB_XHCI_ExpCap_Id_Power 	0x03
#define USB_XHCI_ExpCap_Id_IOVirt	0x04
#define USB_XHCI_ExpCap_Id_Message 	0x05
#define USB_XHCI_ExpCap_Id_LocalMem 0x06
#define USB_XHCI_ExpCap_Id_Debug	0x0a
#define USB_XHCI_ExpCap_Id_MsgIntr	0x11

extern List HW_USB_XHCI_mgrList;

void HW_USB_XHCI_Init(PCIeConfig *xhci);

#endif