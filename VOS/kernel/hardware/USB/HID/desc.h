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

#pragma region Report Parse
#define HID_ReportItem_Tag	(0xfc)
#define HID_ReportItem_Type	(0x02)
#define HID_ReportItem_Type_Main	(0 << 2)
#define HID_ReportItem_Type_Global	(1 << 2)
#define HID_ReportItem_Type_Local	(2 << 2)
#define HID_ReportItem_Size	(0x03)

#define HID_ReportItem_DataType_Const 		(1 << 0)
#define HID_ReportItem_DataType_Vari		(1 << 1)
#define HID_ReportItem_DataType_Relative	(1 << 2)
#define HID_ReportItem_DataType_Wrap		(1 << 3)
#define HID_ReportItem_DataType_NoLinear	(1 << 4)
#define HID_ReportItem_DataType_NoPrefered	(1 << 5)
#define HID_ReportItem_DataType_NullState	(1 << 6)

#pragma endregion

typedef struct USB_HID_ReportItem {
	int rgMin, rgMx, off;
	u8 flags, size;
	// offset == -1 means this item does not exist in a report
} USB_HID_ReportItem;

typedef struct USB_HID_ReportHelper {
	int type, totSz;
	// the raw data of report descriptor
	u8 *raw;
	#define USB_HID_ReportHelper_Type_Mouse 1
	#define USB_HID_ReportHelper_Type_Keyboard 2
	int protoId;
	#define USB_HID_ReportParseHelper_Type_Mouse 0
	union {
		struct {
			USB_HID_ReportItem btn, mvX, mvY, mxZ;
		} mouse;
		union {
			USB_HID_ReportItem spK, k1, k2, k3, k4, k5, k6;
		} keyboard;
	};
} __attribute__ ((packed)) USB_HID_ReportHelper;
extern struct USB_HID_Driver HW_USB_HID_driver;

#endif