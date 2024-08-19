#include "drv.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"

struct USB_HidDriver {
	USB_XHCI_Driver drv;
	USB_HID_EveBuf eveBuf[1024];
	int eveBufHead, eveBufTail;
	SpinLock bufLock;
};

static struct USB_HidDriver _drv;

u64 USB_HID_thread(u64 (*_)(u64), u64 arg) {
	Task_kernelEntryHeader();
	USB_XHCI_Device *dev = (USB_XHCI_Device *)arg;
	printk(WHITE, BLACK, "USB_HID_thread(): dev %#018lx is a hid device\n", dev);
	// setup endpoints of this device
	while (1) IO_hlt();
	Task_kernelThreadExit(0);
}

u64 HW_USB_HID_loader(USB_XHCI_Device *dev) {
	TaskStruct *tsk = Task_createTask(USB_HID_thread, NULL, (u64)dev, Task_Flag_Inner | Task_Flag_Kernel);
	dev->drvTask = tsk;
	return HW_USB_XHCI_DriverCheck_Success;
}

int HW_USB_HID_chk(USB_XHCI_Device *dev) {
	int valid = 0;
	// setup endpoint using descriptors
	for (int i = 0; i < dev->desc->numConfig; i++) {
		for (USB_XHCI_DescHeader *hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[i], NULL); hdr != NULL; hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[i], hdr)) {
			if (hdr->descType != 0x04 || container(hdr, USB_XHCI_InterfaceDesc, header)->interfaceClass != 0x03) continue;
			valid = 1;
			break;
		}
	}
	return valid ? HW_USB_XHCI_DriverCheck_Success : HW_USB_XHCI_DriverCheck_Unmatched;
}

USB_HID_EveBuf HW_USB_HID_popEvent() {
	SpinLock_lock(&_drv.bufLock);
	if (_drv.eveBufHead == _drv.eveBufTail) return (USB_HID_EveBuf){0, {0, 0, 0}};
	USB_HID_EveBuf eve = _drv.eveBuf[_drv.eveBufHead];
	_drv.eveBufHead = (_drv.eveBufHead + 1) % 1024;
	SpinLock_unlock(&_drv.bufLock);
	return eve;
}

void HW_USB_HID_init() {
	_drv.eveBufHead = _drv.eveBufTail = 0;
	_drv.drv.loader = HW_USB_HID_loader;
	_drv.drv.chk = HW_USB_HID_chk;
	_drv.drv.name = "USB HID Basic Driver";
	SpinLock_init(&_drv.bufLock);
	HW_USB_XHCI_addDriver(&_drv.drv);
}
