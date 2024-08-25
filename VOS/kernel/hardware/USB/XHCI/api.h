#ifndef __HW_USB_XHCI_API_H__
#define __HW_USB_XHCI_API_H__

#include "desc.h"

void HW_USB_XHCI_init(PCIeConfig *cfg);

void HW_USB_XHCI_portDetectTask(XHCI_host *host, u64 arg2);

#endif