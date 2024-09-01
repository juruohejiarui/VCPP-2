#ifndef __HW_USB_HID_API_H__
#define __HW_USB_HID_API_H__

#include "desc.h"

void HW_USB_HID_init();

int HW_USB_HID_check(XHCI_Device *dev);
void HW_USB_HID_process(XHCI_Device *dev);

#endif