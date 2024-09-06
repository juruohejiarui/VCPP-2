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
#define HID_RepItem_Size	(0x03)
#define HID_RepItem_Type	(0x0c)

#define HID_RepItem_Type_Main	(0x0)
#define HID_RepItem_Type_Global	(0x1)
#define HID_RepItem_Type_Local	(0x2)
#define HID_RepItem_Type_Long	(0x3)

#define HID_RepItem_Tag		(0xf0)

#define HID_RepItem_Tag_Input	0x08
#define HID_RepItem_Tag_Output	0x09
#define HID_RepItem_Tag_Coll	0x0a
#define HID_RepItem_Tag_Feature	0x0b
#define HID_RepItem_Tag_EndColl	0x0c

#define HID_RepItem_DataType_Const 		(1 << 0)
#define HID_RepItem_DataType_Vari		(1 << 1)
#define HID_RepItem_DataType_Relative	(1 << 2)
#define HID_RepItem_DataType_Wrap		(1 << 3)
#define HID_RepItem_DataType_NoLinear	(1 << 4)
#define HID_RepItem_DataType_NoPrefered	(1 << 5)
#define HID_RepItem_DataType_NullState	(1 << 6)

#pragma endregion

typedef struct USB_HID_ReportItem {
	int rgMn, rgMx, off;
	u8 flags, size;
	// offset == -1 means this item does not exist in a report
} USB_HID_ReportItem;

typedef struct USB_HID_ReportHelper {
	int type, inSz, outSz;
	// the raw data of report descriptor
	u8 *raw;
	#define USB_HID_ReportHelper_Type_Mouse 1
	#define USB_HID_ReportHelper_Type_Keyboard 2
	int protoId;
	#define USB_HID_ReportParseHelper_Type_Mouse 0
	union {
		struct {
			USB_HID_ReportItem btn[3], mvX, mvY, mvZ;
		} mouse;
		union {
			USB_HID_ReportItem spK, k1, k2, k3, k4, k5, k6;
			USB_HID_ReportItem leds;
		} keyboard;
	} items;
} __attribute__ ((packed)) USB_HID_ReportHelper;
extern struct USB_HID_Driver HW_USB_HID_driver;

#endif