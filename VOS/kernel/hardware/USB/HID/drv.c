#include "drv.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"

struct USB_HidDriver {
	USB_XHCI_Driver drv;
	USB_HID_EveBuf eveBuf[1024];
	int eveBufHead, eveBufTail;
};

struct USB_HidDriver drv;

void _setupEndpoints(USB_XHCI_Device *dev) {
	// initailize the configuration and address the device again
	for (int i = 0; i < dev->desc->numConfig; i++) {
		for (USB_XHCI_DescHeader *hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[i], NULL); hdr != NULL; hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[i], hdr)) {
			if (hdr->descType != HW_USB_XHCI_DescType_Interface || container(hdr, USB_XHCI_InterfaceDesc, header)->interfaceClass != 0x03) {
				for (int i = 0; i < hdr->len; i++) printk(WHITE, BLACK, "%02x ", *((u8 *)hdr + i));
				printk(WHITE, BLACK, "\n");
				continue;
			}
			USB_XHCI_InterfaceDesc *desc = container(hdr, USB_XHCI_InterfaceDesc, header);
			printk(WHITE, BLACK, "dev %#018lx: interface %#018lx: subClass:%d proto:%d\n", dev, desc, desc->interfaceSubClass, desc->interfaceProtocol);
		}
	}
	// choose the configuration 0
	dev->ctx->inCtx.addFlags = dev->ctx->inCtx.dropFlags = 0;
	for (USB_XHCI_DescHeader *hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[0], NULL); hdr != NULL; hdr = HW_USB_XHCI_getNxtDesc(dev->cfgDesc[0], hdr)) {
		if (hdr->descType != HW_USB_XHCI_DescType_Endpoint) continue;

		USB_XHCI_EndpointDesc *desc = container(hdr, USB_XHCI_EndpointDesc, header);
		int epId = HW_USB_XHCI_EndpointId(desc->epAddr & ((1u << 7) - 1), (desc->epAddr >> 7) & 1);
		printk(WHITE, BLACK, "enable endpoint %d\n", epId);
		dev->ctx->inCtx.addFlags |= (1 << epId);

		USB_XHCI_EndpointContext *epCtx = &dev->ctx->epCtx[epId];
		memset(epCtx, 0, sizeof(USB_XHCI_EndpointContext));
	}
	// config the endpoint
	USB_XHCI_ReqBlock *reqs = HW_USB_XHCI_mkCmdBlk(HW_USB_TrbType_ConfigEpCmd, dev->slot, (u64)dev->ctx);
	// set configuration
	reqs = HW_USB_XHCI_mkSetCfgBlk(dev->slot, dev->cfgDesc[0]->configVal);
	HW_USB_XHCI_insReqBlk(dev->ctrl, reqs);
	HW_USB_XHCI_waitRely(dev->ctrl, reqs);
	if (reqs->flags & HW_USB_XHCIReq_Flag_failed) printk(RED, BLACK, "fail to set %#018lx to config 0\n", dev);
}

u64 USB_HID_thread(u64 (*_)(u64), u64 arg) {
	Task_kernelEntryHeader();
	USB_XHCI_Device *dev = (USB_XHCI_Device *)arg;
	printk(WHITE, BLACK, "USB_HID_thread(): dev %#018lx is a hid device\n", dev);
	// setup endpoints of this device
	_setupEndpoints(dev);
	while (1) IO_hlt();
	Task_kernelEntryEnd(0);
}

u64 HW_USB_HID_loader(USB_XHCI_Device *dev) {
	Task_createTask(USB_HID_thread, NULL, (u64)dev, Task_Flag_Inner | Task_Flag_Kernel);
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
	if (drv.eveBufHead == drv.eveBufTail) return (USB_HID_EveBuf){0, {0, 0, 0}};
	USB_HID_EveBuf eve = drv.eveBuf[drv.eveBufHead];
	drv.eveBufHead = (drv.eveBufHead + 1) % 1024;
	return eve;
}

void HW_USB_HID_init() {
	drv.eveBufHead = drv.eveBufTail = 0;
	drv.drv.loader = HW_USB_HID_loader;
	drv.drv.chk = HW_USB_HID_chk;
	drv.drv.name = "USB HID Basic Driver";
	HW_USB_XHCI_addDriver(&drv.drv);
}