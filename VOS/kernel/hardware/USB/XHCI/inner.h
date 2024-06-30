#ifndef __HW_USB_XHCI_INNER_H__
#define __HW_USB_XHCI_INNER_H__

#include "../XHCI.h"

#define maxSlots(ctrl) ((ctrl->capRegs->hcsParams1) & 0xff)
#define maxIntrs(ctrl) ((ctrl->capRegs->hcsParams1 >> 8) & 0x7ff)
#define maxPorts(ctrl) ((ctrl->capRegs->hcsParams1 >> 24) & 0xff)
#define extCapPtr(ctrl) ((ctrl->capRegs->hccparam1 >> 16) & 0xffff)
#define ist(ctrl) (ctrl->capRegs->hcsParams2 & 0x7)
#define maxEveRingSegTbl(ctrl) ((ctrl->capRegs->hcsParams2 & 0xf) >> 4);

#define _HCCP1_AC64		0
#define _HCPP1_PWRCTRL	3

#define UsbState_EveIntr		(1 << 3)
#define UsbState_PortChange		(1 << 4)

#define Port_StatusCtrl_ConnectChange 	(1 << 17)
#define Port_StatusCtrl_Power			(1 << 9)
#define Port_StatusCtrl_GenerAllEve		((1 << 25) | (1 << 26) | (1 << 27))
#define Port_StatusCtrl_Reserved		((1 << 2) | (1 << 28) | (1 << 29))

static inline u64 maxScratchBufs(USB_XHCIController *ctrl) {
	u64 higt = ctrl->capRegs->hcsParams2 >> 21 & 0x1f,
		low = ctrl->capRegs->hcsParams2 >> 27 & 0x1f;
	return (higt << 5) | low;
}
#define ac64(ctrl) (ctrl->capRegs->hcsParams3 & 0x1);

static inline void _setCmdBit(USB_XHCIController *ctrl, u32 bit) {
	ctrl->opRegs->usbCmd = (ctrl->opRegs->usbCmd & (~(1u << bit))) | (1u << bit);
}
static inline void _clrCmdBit(USB_XHCIController *ctrl, u32 bit) {
	ctrl->opRegs->usbCmd &= ~(1u << bit);
}

static inline int _rdCmdBit(USB_XHCIController *ctrl, u32 bit) {
	return (ctrl->opRegs->usbCmd >> bit) & 1;
}

static inline int _rdStsBit(USB_XHCIController *ctrl, u32 bit) {
	return (ctrl->opRegs->usbStatus >> bit) & 1;
}

static inline void _setPortStsCtrl(u32 *addr, u32 val) {
	*addr = (*addr & Port_StatusCtrl_Reserved) | val;
}

static inline void _writeDoorbell(USB_XHCIController *ctrl, int slotId, u32 val) {
	__asm__ volatile ("sfence	\n\t": : :);
	ctrl->dbRegs->doorbell[slotId - 1] = val;
}


#endif