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

typedef struct USB_HID_ReportItem {
	int rgMin, rgMx, sz, off;
	// offset == -1 means this item does not exist in a report
} USB_HID_ReportItem;

typedef struct USB_HID_ReportParseHelper {
	int type;
	int protoId;
	#define USB_HID_ReportParseHelper_Type_Mouse 0
	union {
		struct {
			USB_HID_ReportItem btn, mvX, mvY, mxZ;
		} mouse;
	};
} __attribute__ ((packed)) USB_HID_ReportParseHelper;
struct USB_HID_Report {
	u8 *raw; // raw data directly returned from device or which should be sent to device
	USB_HID_ReportParseHelper *parseHelper;
	
} USB_HID_Report;
extern struct USB_HID_Driver HW_USB_HID_driver;

#endif