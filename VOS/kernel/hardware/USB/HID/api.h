#ifndef __HW_USB_HID_API_H__
#define __HW_USB_HID_API_H__

#include "desc.h"

void HW_USB_HID_initParse();
void HW_USB_HID_init();

USB_HID_ReportHelper *HW_USB_HID_genParseHelper(u8 *report, u64 len);
int HW_USB_HID_check(XHCI_Device *dev);
void HW_USB_HID_process(XHCI_Device *dev);

#endif