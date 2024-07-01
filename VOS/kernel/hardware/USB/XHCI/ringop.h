#ifndef __HW_USB_XHCI_RINGOP_H__
#define __HW_USB_XHCI_RINGOP_H__

#include "inner.h"

// get the next Event TRB from event ring, copy it into OUTPUT and return whether the event ring is empty.
// return 0 if the event ring is empty, and return 1 otherwise.
int HW_USB_XHCI_getNextEveTRB(USB_XHCIController *ctrl, int intrId, USB_XHCI_GenerTRB *output);

// get the next cmd ring that should write to, return NULL if the command ring is full
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextCmdTRB(USB_XHCIController *ctrl);
#endif