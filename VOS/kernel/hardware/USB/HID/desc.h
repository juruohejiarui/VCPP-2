#ifndef __HW_USB_HID_DESC_H__
#define __HW_USB_HID_DESC_H__

#include "../XHCI.h"

typedef struct USB_HidDesc {
	XHCI_DescHdr hdr;
	u16 bcdHID;
	u8 bCountryCode;
	u8 bNumDesc;
	u8 bDescType;
	u16 wDescLen;
} __attribute__ ((packed)) USB_HidDesc;

struct USB_HID_Driver {
	XHCI_Driver driver;
};
extern struct USB_HID_Driver HW_USB_HID_driver;

#endif