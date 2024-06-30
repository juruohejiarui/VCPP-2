#include "ringop.h"
#include "../../../includes/log.h"

// get the next Event TRB from event ring
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextEveTRB(USB_XHCIController *ctrl, int intrId) {
	USB_XHCI_GenerTRB *trb = 
		(USB_XHCI_GenerTRB *)DMAS_phys2Virt(ctrl->eveRingSegTbls[intrId][ctrl->eveRingFlag[intrId].segId].addr)
			+ ctrl->eveRingFlag[intrId].pos;
	ctrl->rtRegs->intrRegs[intrId].eveDeqPtr |= 0x8;
	IO_mfence();
	if (trb->dw3.ctx.cycle != ctrl->eveRingFlag[intrId].cycleBit) return NULL;
	// get next ptr
	ctrl->eveRingFlag[intrId].pos++;
	// write the nextPtr
	ctrl->rtRegs->intrRegs[intrId].eveDeqPtr = DMAS_virt2Phys(trb) + sizeof(USB_XHCI_GenerTRB);
	if (ctrl->eveRingFlag[intrId].pos == HW_USB_XHCI_RingEntryNum) {
		// switch to next segment
		ctrl->eveRingFlag[intrId].segId++;
		ctrl->eveRingFlag[intrId].segId %= HW_USB_XHCI_EveRingSegTblSize;
		ctrl->eveRingFlag[intrId].pos = 0;
		if (!ctrl->eveRingFlag[intrId].segId) ctrl->eveRingFlag[intrId].cycleBit ^= 1;
		ctrl->rtRegs->intrRegs[intrId].eveDeqPtr = 
			ctrl->eveRingSegTbls[intrId][ctrl->eveRingFlag[intrId].segId].addr;
	}
	IO_mfence();
	return trb;
}

// get the next cmd ring that should write to, return NULL if the command ring is full
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextCmdTRB(USB_XHCIController *ctrl) {
	USB_XHCI_GenerTRB *trb = &ctrl->cmdRing[ctrl->cmdRingFlag.pos];
	// should loop back
	if (trb->dw3.ctx.trbType == HW_USB_TrbType_Link) {
		ctrl->cmdRingFlag.cycleBit ^= 1;
		IO_mfence();
		ctrl->cmdRingFlag.pos = 0;
		trb = ctrl->cmdRing;
	}
	if ((ctrl->cmdsFlag[ctrl->cmdRingFlag.pos] & 1) == 0) return NULL;
	ctrl->cmdsFlag[ctrl->cmdRingFlag.pos] ^= 1;
	ctrl->cmdRingFlag.pos++;
	return trb;
}