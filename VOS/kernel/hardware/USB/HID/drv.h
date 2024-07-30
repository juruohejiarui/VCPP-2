#ifndef __HW_USB_HID_DRV_H__
#define __HW_USB_HID_DRV_H__

#include "../XHCI.h"

#define USB_HID_EventType_Mouse		1
#define USB_HID_EventType_Keyboard	2

typedef struct USB_HID_EvtBuf {
	int type;
	int val[3];
} USB_HID_EveBuf;

u64 USB_HID_task(u64 (*usrEntry)(u64), u64 arg);

int USB_HID_chk(USB_XHCI_Device *dev);

USB_HID_EveBuf USB_HID_popEvent();

void USB_HID_init();

#endif