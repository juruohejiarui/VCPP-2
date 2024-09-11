#ifndef __HW_USB_HID_API_H__
#define __HW_USB_HID_API_H__

#include "desc.h"

void HW_USB_HID_initParse();
void HW_USB_HID_init();

USB_HID_ParseHelper *HW_USB_HID_genParseHelper(u8 *report, u64 len);
void HW_USB_HID_parseReport(u8 *raw, USB_HID_ParseHelper *helper, USB_HID_Report *out);

int HW_USB_HID_getIdleDuration(int repType);

// set a SET_REPORT to control endpoint and return the completion code
int HW_USB_HID_setReport(XHCI_Device *dev, u8 reportId, u8 interId, u8 *report, u8 repLen);

int HW_USB_HID_check(XHCI_Device *dev);
void HW_USB_HID_process(XHCI_Device *dev);

#endif