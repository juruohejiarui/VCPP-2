#ifndef __HW_USB_XHCI_DESC_H__
#define __HW_USB_XHCI_DESC_H__

#include "../../../includes/lib.h"
#include "../../PCIe.h"

typedef struct XHCI_host {
	List listEle;
} XHCI_host;

extern List HW_USB_XHCI_hostList;
#endif