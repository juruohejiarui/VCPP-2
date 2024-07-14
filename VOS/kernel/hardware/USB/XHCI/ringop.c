#include "ringop.h"
#include "../../../includes/log.h"

// get the next Event TRB from event ring, copy it into OUTPUT and return whether the event ring is empty.
// return 0 if the event ring is empty, and return 1 otherwise.
int HW_USB_XHCI_getNextEveTRB(USB_XHCIController *ctrl, int intrId, USB_XHCI_GenerTRB *output) {
	USB_XHCI_GenerTRB *trb = 
		(USB_XHCI_GenerTRB *)DMAS_phys2Virt(ctrl->eveRingSegTbls[intrId][ctrl->eveRingFlag[intrId].segId].addr)
			+ ctrl->eveRingFlag[intrId].pos;
	if (trb->dw3.ctx.cycle != ctrl->eveRingFlag[intrId].cycleBit) return 0;
	if (output != NULL) memcpy(trb, output, sizeof(USB_XHCI_GenerTRB));
	IO_mfence();
	// get next ptr
	ctrl->eveRingFlag[intrId].pos++;
	// write the nextPtr
	u64 val = 0x8;
	if (ctrl->eveRingFlag[intrId].pos == HW_USB_XHCI_RingEntryNum) {
		// switch to next segment
		ctrl->eveRingFlag[intrId].segId++;
		ctrl->eveRingFlag[intrId].segId %= HW_USB_XHCI_EveRingSegTblSize;
		ctrl->eveRingFlag[intrId].pos = 0;
		if (!ctrl->eveRingFlag[intrId].segId) ctrl->eveRingFlag[intrId].cycleBit ^= 1;
		val |= ctrl->eveRingSegTbls[intrId][ctrl->eveRingFlag[intrId].segId].addr;
	} else
		val |= (DMAS_virt2Phys(trb) + sizeof(USB_XHCI_GenerTRB));
	ctrl->rtRegs->intrRegs[intrId].eveDeqPtr = val;
	IO_mfence();
	return 1;
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
	if (ctrl->cmdSrc[ctrl->cmdRingFlag.pos] != NULL) return NULL;
	ctrl->cmdRingFlag.pos++;
	memset(trb, 0, sizeof(USB_XHCI_GenerTRB));
	return trb;
}

USB_XHCI_GenerTRB *HW_USB_XHCI_allocTransferRing(USB_XHCIController *ctrl, USB_XHCI_LinkTRB *fr, USB_XHCI_GenerTRB *to) {
	USB_XHCI_GenerTRB *ring = HW_USB_XHCI_alloc(ctrl, Page_4KSize * 16);
	// construct the link trb at the end of this ring
	USB_XHCI_LinkTRB *lkTrb = (USB_XHCI_LinkTRB *)(ring + HW_USB_XHCI_RingEntryNum - 1);
	lkTrb->dw3.raw |= 3;
	lkTrb->dw3.ctx.trbType = HW_USB_TrbType_Link;
	lkTrb->ptr = DMAS_virt2Phys(to == NULL ? ring : to);
	if (fr != NULL) fr->ptr = DMAS_virt2Phys(ring);
	return ring;
}

USB_XHCI_GenerTRB *HW_USB_XHCI_getNextTransferTRB(USB_XHCIController *ctrl, int slotId, int ep) {
	USB_XHCI_Device *dev = ctrl->devices[slotId];
    USB_XHCI_GenerTRB *trb = dev->transInqPtr[ep], *nxt = trb + 1;
	int pos = HW_USB_getRingPos(trb);

	printk(ORANGE, BLACK, "XHCI: %#018lx: slot:%d ep:%d -> trb=%#018lx,pos:%d\n", ctrl, slotId, ep, trb, pos);
	
	// there is an unfinished transfer
	if (dev->transSrc[ep][pos] != NULL) return NULL;

	if (nxt->dw3.ctx.trbType == HW_USB_TrbType_Link) {
		// if toggle the cycle bit.
		if (nxt->dw3.raw & 0x2) dev->transCycFlags[ep] ^= 1;
		dev->transInqPtr[ep] = DMAS_phys2Virt(*(u64 *)&nxt->dw[0]);
	} else dev->transInqPtr[ep] = nxt;

	return trb;
}