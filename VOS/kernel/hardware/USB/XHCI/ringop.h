#ifndef __HW_USB_XHCI_RINGOP_H__
#define __HW_USB_XHCI_RINGOP_H__

#include "inner.h"

// get the next Event TRB from event ring
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextEveTRB(USB_XHCIController *ctrl, int intrId);

// get the next cmd ring that should write to, return NULL if the command ring is full
USB_XHCI_GenerTRB *HW_USB_XHCI_getNextCmdTRB(USB_XHCIController *ctrl);
#endif