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
	HW_USB_HID_initParse();
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

// make a parse helper from the hid report descriptor referred by a specific hid descriptor
USB_HID_ReportHelper *HW_USB_HID_mkParseHelper(XHCI_Device *dev, XHCI_InterDesc *inter, USB_HidDesc *desc) {
	XHCI_Request *req = HW_USB_XHCI_allocReq(3);
	u8 *reportDesc = kmalloc(0xff, Slab_kmalloc_arg_Clear | Slab_kmalloc_arg_Private, NULL);
	{
		XHCI_GenerTRB *setup = &req->trb[0];
		HW_USB_XHCI_TRB_setData(setup, 		HW_USB_XHCI_TRB_mkSetup(0x81, 0x6, 0x2200 | 0, inter->bInterNum, 0xff));
		HW_USB_XHCI_TRB_setStatus(setup, 	HW_USB_XHCI_TRB_mkStatus(8, 0, 0));
		HW_USB_XHCI_TRB_setType(setup, 		XHCI_TRB_Type_SetupStage);
		HW_USB_XHCI_TRB_setCtrlBit(setup, 	XHCI_TRB_Ctrl_idt);
		HW_USB_XHCI_TRB_setTRT(setup, 		XHCI_TRB_TRT_In);
	}
	{
		XHCI_GenerTRB *data = &req->trb[1];
		HW_USB_XHCI_TRB_setData(data,	DMAS_virt2Phys(reportDesc));
		HW_USB_XHCI_TRB_setStatus(data, HW_USB_XHCI_TRB_mkStatus(0xff, 0, 0));
		HW_USB_XHCI_TRB_setType(data, 	XHCI_TRB_Type_DataStage);
		HW_USB_XHCI_TRB_setDir(data, 	XHCI_TRB_Ctrl_Dir_In);
	}
	{
		XHCI_GenerTRB *status = &req->trb[2];
		HW_USB_XHCI_TRB_setType(status, 	XHCI_TRB_Type_StatusStage);
		HW_USB_XHCI_TRB_setCtrlBit(status,	XHCI_TRB_Ctrl_ioc);
	}
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to get report descriptor, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req->res));
		while (1) IO_hlt();
	}
	for (int i = 0; i < desc->wDescLen; i++) printk(WHITE, BLACK, "%02x ", reportDesc[i]);
	printk(WHITE, BLACK, "\n");
	kfree(req, Slab_kmalloc_arg_Private);

	return HW_USB_HID_genParseHelper(reportDesc, desc->wDescLen);
}

void HW_USB_HID_process(XHCI_Device *dev) {
	// get a correct interface
	// the interface with class=0x03 and subClass=0x00 is the best one
	// the interface with class=0x03 and subClass=0x01 is the second best
	XHCI_InterDesc *bstInter = NULL;
	int bstRate = -1, epId, epType;
	printk(BLUE, BLACK, "dev %#018lx accept HID Driver\n", dev);
	for (XHCI_DescHdr *hdr = &dev->cfgDesc[0]->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr))
		if (hdr->type == XHCI_Descriptor_Type_Inter) {
			XHCI_InterDesc *cur = container(hdr, XHCI_InterDesc, hdr); 
			int curRate = (cur->bInterClass == 0x03 ? 1 : 0) + (cur->bInterSubClass == 0x00 ? 1 : 0);
			printk(WHITE, BLACK, "dev %#018lx: interface: class:%#04x subClass:%#04x proto:%#04x\n", 
				dev, cur->bInterClass, cur->bInterSubClass, cur->bInterProto);
			if (curRate > bstRate) bstInter = cur, bstRate = curRate;
		}
	// Normally, there will be only one endpoint for one interface
	XHCI_EpDesc *epDesc = NULL;
	USB_HidDesc *hidDesc = NULL;
	USB_HID_ReportHelper *repHelper = NULL;
	for (XHCI_DescHdr *hdr = &bstInter->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr)) {
		switch (hdr->type) {
			case XHCI_Descriptor_Type_Inter :
				// has been moved to next interface descriptor
				if (hdr != &bstInter->hdr) goto EndOfScanningDesc;
				break;
			case XHCI_Descriptor_Type_Endpoint :
				epDesc = container(hdr, XHCI_EpDesc, hdr);
				break;
			case XHCI_Descriptor_Type_HID :
				hidDesc = container(hdr, USB_HidDesc, hdr);
				break;
		}
	}
	// get the report descriptor of hidDesc exists
	EndOfScanningDesc:
	if (hidDesc) repHelper = HW_USB_HID_mkParseHelper(dev, bstInter, hidDesc);
	
	if (repHelper == NULL || !repHelper->type) {
		printk(RED, BLACK, "dev %#018lx: unsupported HID device\n", dev);
		Task_setSignal(Task_current, Task_Signal_Int);
		while (1) IO_hlt();
	}
	epId = ((epDesc->bEpAddr & 0xf) << 1) + (epDesc->bEpAddr >> 7) - 1;
	epType = (epDesc->bmAttr & 0x3) | ((epDesc->bEpAddr >> 5) & 0x4);
	XHCI_EpCtx *ep = &dev->inCtx->ep[epId];
	memset(ep, 0, sizeof(XHCI_EpCtx));

	printk(WHITE, BLACK, "\tepId:%d epType:%d mxPackSz:%d mxBurstSize:%d interval:%d\n", 
	epId, epType, epDesc->wMxPackSz & 0x07ff, (epDesc->wMxPackSz & 0x1800) >> 11, epDesc->interval);

	HW_USB_XHCI_writeCtx(&dev->inCtx->slot, 0, XHCI_SlotCtx_ctxEntries, epId + 1);

	HW_USB_XHCI_writeCtx(ep, 0, XHCI_EpCtx_interval,	epDesc->interval);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_epType, 		epType);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_CErr, 		3);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxPackSize, 	epDesc->wMxPackSz & 0x07ff);
	HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxBurstSize,	(epDesc->wMxPackSz & 0x1800) >> 11);

	dev->trRing[epId] = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
	XHCI_GenerTRB *lk = &dev->trRing[epId]->ring[XHCI_Ring_maxSize - 1];
	HW_USB_XHCI_TRB_setData(lk, DMAS_virt2Phys(&dev->trRing[epId]->ring[0]));
	HW_USB_XHCI_TRB_setType(lk, XHCI_TRB_Type_Link);
	HW_USB_XHCI_TRB_setToggle(lk, 1);
	ep->deqPtr = DMAS_virt2Phys(dev->trRing[epId]->cur) | 1;

	HW_USB_XHCI_writeCtx(ep, 4, XHCI_EpCtx_aveTrbLen, (1 << 10));

	HW_USB_XHCI_EpCtx_writeMxESITPay(ep, 
		HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxPackSize) * (HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxBurstSize) + 1));

	dev->inCtx->ctrl.addFlags = 1 | (1u << (epId + 1));

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
	HW_USB_XHCI_TRB_setTRT(&req1->trb[0], XHCI_TRB_TRT_In);

	HW_USB_XHCI_TRB_setDir(&req1->trb[1], XHCI_TRB_Ctrl_Dir_In);
	HW_USB_XHCI_TRB_setType(&req1->trb[1], XHCI_TRB_Type_StatusStage);
	HW_USB_XHCI_TRB_setCtrlBit(&req1->trb[1], XHCI_TRB_Ctrl_ioc);

	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set configuration, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	printk(BLUE, BLACK, "dev %#018lx: set configuration successfully\n", dev);
	// set SET_INTERFACE request to device
	HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x01, 0x0b, bstInter->bAlterSet, bstInter->bInterNum, 0));
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set interface, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	// set Idle
	HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x21, 0x0a, (repHelper->type == USB_HID_ReportHelper_Type_Mouse ? 0xff00 : 0x100), 0, 0));
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set idle, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	kfree(req0, Slab_kmalloc_arg_Private);
	kfree(req1, Slab_kmalloc_arg_Private);
	req1 = HW_USB_XHCI_allocReq(1);

	u8 *report = kmalloc(0xff, Slab_kmalloc_arg_Private | Slab_kmalloc_arg_Clear, NULL);
	printk(WHITE, BLACK, "create report on %#018lx\n", report);
	HW_USB_XHCI_TRB_setData(&req1->trb[0], DMAS_virt2Phys(report));
	HW_USB_XHCI_TRB_setStatus(&req1->trb[0], HW_USB_XHCI_TRB_mkStatus(repHelper->inSz / 8, 0x0, 0));
	HW_USB_XHCI_TRB_setType(&req1->trb[0], XHCI_TRB_Type_Normal);
	HW_USB_XHCI_TRB_setCtrlBit(&req1->trb[0], XHCI_TRB_Ctrl_ioc | XHCI_TRB_Ctrl_isp);
	// start to get report from the endpoint
	while (1) {	
		HW_USB_XHCI_Ring_insReq(dev->trRing[epId], req1);
		register int res = HW_USB_XHCI_Req_ringDoorbellWait(dev->host, dev->slotId, epId + 1, 0, req1);
		if (res != XHCI_TRB_CmplCode_Succ && res != XHCI_TRB_CmplCode_ShortPkg) {
			printk(RED, BLACK, "dev %#018lx: get report failed, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
			while (1) IO_hlt(); 
		}
		for (int i = 0; i < repHelper->inSz / 8; i++) printk(WHITE, BLACK, "%02x ", report[i]);
		printk(WHITE, BLACK, "\n");
		// Intr_SoftIrq_Timer_mdelay(max(1, epDesc->interval - 2));
	}
	printk(BLUE, BLACK, "dev %#018lx: set interface successfully\n", dev);
	while (1) IO_hlt();
} 