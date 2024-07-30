#include "drv.h"
#include "../../../includes/task.h"

struct USB_HidDriver {
	USB_XHCI_Driver drv;
	USB_HID_EveBuf eveBuf[1024];
	int eveBufHead, eveBufTail;
};

struct USB_HidDriver drv;

USB_HID_EveBuf USB_HID_popEvent() {
	if (drv.eveBufHead == drv.eveBufTail) return (USB_HID_EveBuf){0, {0, 0, 0}};
	USB_HID_EveBuf eve = drv.eveBuf[drv.eveBufHead];
	drv.eveBufHead = (drv.eveBufHead + 1) % 1024;
	return eve;
}

void USB_HID_init()
{
	drv.drv.task = USB_HID_task;
	drv.drv.taskFlags = Task_Flag_Inner | Task_Flag_Kernel;
	drv.drv.chk = USB_HID_chk;
	HW_USB_XHCI_addDriver(&drv.drv);
}