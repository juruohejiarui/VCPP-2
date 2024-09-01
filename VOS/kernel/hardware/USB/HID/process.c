#include "api.h"
#include "../../../includes/log.h"

struct USB_HID_Driver HW_USB_HID_driver;

void HW_USB_HID_init() {
	memset(&HW_USB_HID_driver, 0, sizeof(struct USB_HID_Driver));
	HW_USB_HID_driver.driver.name = "HID Basic Driver";
	HW_USB_HID_driver.driver.check = HW_USB_HID_check;
	HW_USB_HID_driver.driver.process = HW_USB_HID_process;
	List_init(&HW_USB_HID_driver.driver.list);
	SpinLock_lock(&HW_USB_XHCI_DriverListLock);
	List_insBefore(&HW_USB_HID_driver.driver.list, &HW_USB_XHCI_DriverList);
	SpinLock_unlock(&HW_USB_XHCI_DriverListLock);
}

int HW_USB_HID_check(XHCI_Device *dev) {
	// parse the configuration descriptor
	for (XHCI_DescHdr *hdr = &dev->cfgDesc[0]->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr)) {
		if (hdr->type == XHCI_Descriptor_Type_Inter) {
			XHCI_InterDesc *inter = container(hdr, XHCI_InterDesc, hdr);
			if (inter->bInterClass == 0x03) return 1;
		}
	}
	return 0;
}

void HW_USB_HID_process(XHCI_Device *dev) {
	// get a correct interface 
	// the interface with class=0x03 and subClass=0x00 is the best one
	// the interface with class=0x03 and subClass=0x01 is the second best
	XHCI_InterDesc *bstInter = NULL;
	int bstRate = 0;
	printk(BLUE, BLACK, "dev %#018lx accept HID Driver\n", dev);
	for (XHCI_DescHdr *hdr = &dev->cfgDesc[0]->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr))
		if (hdr->type == XHCI_Descriptor_Type_Inter) {
			XHCI_InterDesc *cur = container(hdr, XHCI_InterDesc, hdr);
			int curRate = (cur->bInterClass == 0x03) + (cur->bInterSubClass == 0x00);
			printk(WHITE, BLACK, "dev %#018lx: endpoint: class:%#04x subClass:%#04x\n", dev, cur->bInterClass, cur->bInterSubClass);
			if (curRate > bstRate) bstInter = cur, bstRate = curRate;
		}
	// get the report descriptor and parse it if subClass=0x00
	// initialize the endpoint(s)
	// Normally, there will be only one endpoint for one interface
	// modify the endpoint using "Configure Endpoint Command"
	// set SET_CONFIGURATION request to device
} 