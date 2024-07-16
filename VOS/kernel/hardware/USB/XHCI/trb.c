#include "inner.h"
#include "trb.h"

USB_XHCI_EventDataBuffer *HW_USB_XHCI_makeEveDataBuf(u64 size) {
	USB_XHCI_EventDataBuffer *buf = kmalloc(size + sizeof(USB_XHCI_EventDataBuffer), 0);
	memset(buf, 0, size + sizeof(USB_XHCI_EventDataBuffer));
	return buf;
}
