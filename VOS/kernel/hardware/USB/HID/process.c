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
	int bstRate = -1;
	printk(BLUE, BLACK, "dev %#018lx accept HID Driver\n", dev);
	for (XHCI_DescHdr *hdr = &dev->cfgDesc[0]->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr))
		if (hdr->type == XHCI_Descriptor_Type_Inter) {
			XHCI_InterDesc *cur = container(hdr, XHCI_InterDesc, hdr);
			int curRate = (cur->bInterClass == 0x03 ? 1 : 0) + (cur->bInterSubClass == 0x00 ? 1 : 0);
			printk(WHITE, BLACK, "dev %#018lx: interface: class:%#04x subClass:%#04x\n", dev, cur->bInterClass, cur->bInterSubClass);
			if (curRate > bstRate) bstInter = cur, bstRate = curRate;
		}
	// get the report descriptor and parse it if subClass=0x00
	// initialize the endpoint(s)
	// Normally, there will be only one endpoint for one interface
	XHCI_EpDesc *epDesc = NULL;
	for (XHCI_DescHdr *hdr = &bstInter->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr))
		if (hdr->type == XHCI_Descriptor_Type_Endpoint) {
			epDesc = container(hdr, XHCI_EpDesc, hdr);
			break;
		}
	int epId = ((epDesc->bEpAddr & 0xf) << 1) + (epDesc->bEpAddr >> 7) - 1;
	int epType = (epDesc->bmAttr & 0x3) | (epDesc->bEpAddr >> 5);
	printk(WHITE, BLACK, "\tepId:%d epType:%d\n", epId, epType);
	XHCI_EpCtx *ep = &dev->inCtx->ep[epId];

	HW_USB_XHCI_writeCtx(ep, 0, XHCI_EpCtx_lsa, 		1);
	HW_USB_XHCI_writeCtx(ep, 0, XHCI_EpCtx_interal,		epDesc->interval);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_epType, 		epType);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_CErr, 		1);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxPackSize, 	epDesc->wMxPackSz & 0x07ff);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxBurstSize,	(epDesc->wMxPackSz & 0x1800) >> 1);


	dev->trRing[epId] = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
	XHCI_GenerTRB *lk = &dev->trRing[epId]->ring[XHCI_Ring_maxSize - 1];
	HW_USB_XHCI_TRB_setData(lk, DMAS_virt2Phys(&dev->trRing[epId][0]));
	HW_USB_XHCI_TRB_setType(lk, XHCI_TRB_Type_Link);
	HW_USB_XHCI_TRB_setToggle(lk, 1);

	ep->deqPtr = DMAS_virt2Phys(dev->trRing[epId]->cur) | 1;
	HW_USB_XHCI_EpCtx_writeMxESITPay(ep, 
		HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxPackSize) * (HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxBurstSize) + 1));

	dev->inCtx->ctrl.addFlags = (1u << (epId + 1));

	// modify the endpoint using "Configure Endpoint Command"
	XHCI_Request *req0 = HW_USB_XHCI_allocReq(1);
	HW_USB_XHCI_TRB_setData(&req0->trb[0], DMAS_virt2Phys(dev->inCtx));
	HW_USB_XHCI_TRB_setSlot(&req0->trb[0], dev->slotId);
	HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_CfgEp);

	HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req0);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, 0, 0, 0, req0) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to configure endpoint %d, code=%d\n", 
			dev, epId, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
		while (1) IO_hlt();
	}
	printk(BLUE, BLACK, "dev %#018lx: configure endpoint %d, status:%d\n", 
		dev, epId, HW_USB_XHCI_readCtx(&dev->ctx->ep[epId], 0, XHCI_EpCtx_epState));

	// set SET_CONFIGURATION request to device
	XHCI_Request *req1 = HW_USB_XHCI_allocReq(2);
	HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x00, 0x09, dev->cfgDesc[0]->bCfgVal, 0, 0));
	HW_USB_XHCI_TRB_setStatus(&req1->trb[0], HW_USB_XHCI_TRB_mkStatus(8, 0, 0));
	HW_USB_XHCI_TRB_setType(&req1->trb[0], XHCI_TRB_Type_SetupStage);
	HW_USB_XHCI_TRB_setCtrlBit(&req1->trb[0], XHCI_TRB_Ctrl_idt);

	HW_USB_XHCI_TRB_setDir(&req1->trb[1], XHCI_TRB_Ctrl_Dir_In);
	HW_USB_XHCI_TRB_setType(&req1->trb[1], XHCI_TRB_Type_StatusStage);
	HW_USB_XHCI_TRB_setCtrlBit(&req1->trb[1], XHCI_TRB_Ctrl_ioc);

	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set configuration, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	printk(BLUE, BLACK, "dev %#018lx: set configuration successfully\n");
	// set SET_INTERFACE request to device
	while (1) IO_hlt();
} 