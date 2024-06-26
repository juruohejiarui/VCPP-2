#ifndef __HW_USB_XHCI_RINGOP_H__
#define __HW_USB_XHCI_RINGOP_H__

#include "inner.h"

// get the next Event TRB from event ring
static USB_XHCI_GenerTRB *_getNextEventTRB(USB_XHCIController *ctrl, int intrId);

// get the next cmd ring that should write to, return NULL if the command ring is full
static USB_XHCI_GenerTRB *_getNextCmdTRB(USB_XHCIController *ctrl);
#endif