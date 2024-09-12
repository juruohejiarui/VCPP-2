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

// tags for main items
#define HID_RepItem_Tag_Input	0x08
#define HID_RepItem_Tag_Output	0x09
#define HID_RepItem_Tag_Coll	0x0a
#define HID_RepItem_Tag_Feature	0x0b
#define HID_RepItem_Tag_EndColl	0x0c

// tags for local items
#define HID_RepItem_Tag_Usage		0x00
#define HID_RepItem_Tag_UsageMin	0x01
#define HID_RepItem_Tag_UsageMax	0x02

// tags for global items
#define HID_RepItem_Tag_UsagePage	0x00
#define HID_RepItem_Tag_LogicalMin	0x01
#define HID_RepItem_Tag_LogicalMax	0x02
#define HID_RepItem_Tag_ReportSize	0x07
#define HID_RepItem_Tag_ReportId	0x08
#define HID_RepItem_Tag_ReportCnt	0x09

#define HID_RepItem_DataType_Const 		(1 << 0)
#define HID_RepItem_DataType_Vari		(1 << 1)
#define HID_RepItem_DataType_Relative	(1 << 2)
#define HID_RepItem_DataType_Wrap		(1 << 3)
#define HID_RepItem_DataType_NoLinear	(1 << 4)
#define HID_RepItem_DataType_NoPrefered	(1 << 5)
#define HID_RepItem_DataType_NullState	(1 << 6)

#define HID_UsagePage_GenerDeskCtrl		(0x01)
#define HID_UsagePage_Keyboard			(0x07)
#define HID_UsagePage_Button			(0x09)


#define HID_Usage_Pointer	(0x01)
#define HID_Usage_Mouse		(0x02)
#define HID_Usage_Keyboard	(0x06)
#define HID_Usage_Keypad	(0x07)
#define HID_Usage_X			(0x30)
#define HID_Usage_Y			(0x31)
#define HID_Usage_Z			(0x32)
#define HID_Usage_Wheel		(0x38)

#define HID_RepItem_Main_isConst(flag)	((flag) & 1)
#define HID_RepItem_Main_isRel(flag)	(((flag) >> 2) & 1)

#pragma endregion

typedef struct USB_HID_ReportItem {
	int rgMn, rgMx;
	u8 flags, size, off;
	// offset == -1 means this item does not exist in a report
} USB_HID_ReportItem;

typedef struct USB_HID_ParseHelper {
	int type, id, inSz, outSz;
	// the raw data of report descriptor
	u8 *raw;
	#define USB_HID_ReportHelper_Type_Mouse		1
	#define USB_HID_ReportHelper_Type_Keyboard 	2
	#define USB_HID_ReportHelper_Type_Touchpad	3
	union {
		struct {
			USB_HID_ReportItem btn, x, y, wheel;
		} mouse;
		struct {
			USB_HID_ReportItem btn, x, y, wheel;
		} touchpad;
		union {
			USB_HID_ReportItem spK, key[6];
			USB_HID_ReportItem leds;
		} keyboard;
	} items;
} __attribute__ ((packed)) USB_HID_ParseHelper;

// the report after parsing
typedef struct USB_HID_Report {
	// the report type which is the same as that of parse helper applied to this report
	int type;
	union {
		struct { int btn, x, y, wheel; } mouse;
		struct { int btn, x, y, wheel; } touchpad;
		struct { int spK, key[6]; } keyboard;
	} items;
} USB_HID_Report;

typedef struct USB_HID_Interface {
	XHCI_InterDesc *desc;
	USB_HidDesc *hidDesc;
	XHCI_EpDesc **eps;
	USB_HID_ParseHelper *helper;
} USB_HID_Interface;


extern struct USB_HID_Driver HW_USB_HID_driver;

#endif