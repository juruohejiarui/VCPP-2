#ifndef __HW_USB_XHCI_RINGOP_H__
#define __HW_USB_XHCI_RINGOP_H__

#include "inner.h"

// get the next Event TRB from event ring, copy it into OUTPUT and return whether the event ring is empty.
// return 0 if the event ring is empty, and return 1 otherwise.
int HW_USB_XHCI_getNextEveTRB(USB_XHCIController *ctrl, int intrId, USB_XHCI_GenerTRB *output);

// get the next cmd ring that should write to, return NULL if the command ring is full
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextCmdTRB(USB_XHCIController *ctrl);

// allocate a transfer ring and set the entry link trb to this ring and where should the pointer goes.
// if to == NULL, then the link trb at the end of this ring points to the beginning to this ring.
USB_XHCI_GenerTRB *HW_USB_XHCI_allocTransferRing(USB_XHCIController *ctrl, USB_XHCI_LinkTRB *fr, USB_XHCI_GenerTRB *to);

// get the next transfer TRB from the transfer ring of EP-th endpoint of the SLOT_ID-th slot. 
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextTransferTRB(USB_XHCIController *ctrl, int slotId, int ep);

#endif