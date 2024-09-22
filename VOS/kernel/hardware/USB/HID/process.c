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

int HW_USB_HID_setReport(XHCI_Device *dev, u8 reportId, u8 interId, u8 *report, u8 repLen) {
	XHCI_Request *req = HW_USB_XHCI_allocReq(3);
	HW_USB_XHCI_ctrlDataReq(req, 
			HW_USB_XHCI_TRB_mkSetup(0x21, 0x09, 0x0200 | reportId, interId, repLen), 
			XHCI_TRB_Ctrl_Dir_Out, 
			report, repLen);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set report, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req->res));
	}
	int res = HW_USB_XHCI_TRB_getCmplCode(&req->res);
	kfree(req, Slab_Flag_Private);
	return res;
}

int HW_USB_HID_getReport(XHCI_Device *dev, u8 reportId, u8 interId, u8 *report, u8 repLen) {
	XHCI_Request *req = HW_USB_XHCI_allocReq(3);
	HW_USB_XHCI_ctrlDataReq(req, 
			HW_USB_XHCI_TRB_mkSetup(0xa1, 0x01, 0x0100 | reportId, interId, repLen), 
			XHCI_TRB_Ctrl_Dir_In, 
			report, repLen);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set report, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req->res));
	}
	int res = HW_USB_XHCI_TRB_getCmplCode(&req->res);
	kfree(req, Slab_Flag_Private);
	return res;
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
USB_HID_ParseHelper *HW_USB_HID_mkParseHelper(XHCI_Device *dev, XHCI_InterDesc *inter, USB_HidDesc *desc) {
	XHCI_Request *req = HW_USB_XHCI_allocReq(3);
	u8 *reportDesc = kmalloc(0xff, Slab_Flag_Clear | Slab_Flag_Private, NULL);
	HW_USB_XHCI_ctrlDataReq(req, 
			HW_USB_XHCI_TRB_mkSetup(0x81, 0x06, 0x2200 | 0, inter->bInterNum, 0xff),
			XHCI_TRB_Ctrl_Dir_In,
			reportDesc, 0xff);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to get report descriptor, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req->res));
		while (1) IO_hlt();
	}
	kfree(req, Slab_Flag_Private);

	return HW_USB_HID_genParseHelper(reportDesc, desc->wDescLen);
}

void HW_USB_HID_processMouse(XHCI_Device *dev, XHCI_InterDesc *inter, USB_HID_ParseHelper *helper, int inEpId, int outEpId, int inInterval) {
	XHCI_Request *req0 = HW_USB_XHCI_allocReq(2), *req1 = HW_USB_XHCI_allocReq(1);

	HW_USB_XHCI_ctrlReq(req0, HW_USB_XHCI_TRB_mkSetup(0x21, 0x0a, 0xff00, inter->bInterNum, 0), XHCI_TRB_Ctrl_Dir_Out);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req0);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req0) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set idle, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
		while (1) IO_hlt();
	}
	kfree(req0, Slab_Flag_Private);

	u8 *repRaw = kmalloc(0xff, Slab_Flag_Private | Slab_Flag_Clear, NULL);
	USB_HID_Report *rep = kmalloc(sizeof(USB_HID_Report), Slab_Flag_Private | Slab_Flag_Private, NULL);

	HW_USB_XHCI_TRB_setData(&req1->trb[0], DMAS_virt2Phys(repRaw));
	HW_USB_XHCI_TRB_setStatus(&req1->trb[0], HW_USB_XHCI_TRB_mkStatus(helper->inSz / 8, 0x0, 0));
	HW_USB_XHCI_TRB_setType(&req1->trb[0], XHCI_TRB_Type_Normal);
	HW_USB_XHCI_TRB_setCtrlBit(&req1->trb[0], XHCI_TRB_Ctrl_ioc);

	// start to get report from the endpoint
	while (1) {	
		HW_USB_XHCI_Ring_insReq(dev->trRing[inEpId], req1);
		register int res = HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, inEpId + 1, 0, req1);
		if (res != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: get report failed, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
			while (1) IO_hlt(); 
		}
		HW_USB_HID_parseReport(repRaw, helper, rep);
		printk(WHITE, BLACK, "M b:%d x:%d y:%d w:%d\r", 
			rep->items.mouse.btn, rep->items.mouse.x, rep->items.mouse.y, rep->items.mouse.wheel);
	}
}

void HW_USB_HID_processKeyboard(XHCI_Device *dev, XHCI_InterDesc *inter, USB_HID_ParseHelper *helper, int inEpId, int outEpId, int inInterval) {
	XHCI_Request *req0;
	req0 = HW_USB_XHCI_allocReq(1);
	u8 *repRaw = kmalloc(0xff, Slab_Flag_Private | Slab_Flag_Clear, NULL);
	USB_HID_Report *rep = kmalloc(sizeof(USB_HID_Report), Slab_Flag_Private | Slab_Flag_Private, NULL);

	HW_USB_XHCI_TRB_setData(&req0->trb[0], DMAS_virt2Phys(repRaw));
	HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_Normal);
	HW_USB_XHCI_TRB_setCtrlBit(&req0->trb[0], XHCI_TRB_Ctrl_ioc);

	repRaw[0] = (1 << 4);
	// set SET_REPORT to enable default led
	if (outEpId != -1) {
		HW_USB_XHCI_TRB_setStatus(&req0->trb[0], HW_USB_XHCI_TRB_mkStatus(helper->outSz / 8, 0x0, 0));
		HW_USB_XHCI_Ring_insReq(dev->trRing[outEpId], req0);
		register int res = HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, outEpId + 1, 0, req0);
		if (res != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: set report failed, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
			while (1) IO_hlt();
		}
	} else HW_USB_HID_setReport(dev, 0, inter->bInterNum, repRaw, 1);
	HW_USB_XHCI_TRB_setStatus(&req0->trb[0], HW_USB_XHCI_TRB_mkStatus(helper->inSz / 8, 0x0, 0));
	// start to get report from the endpoint
	while (1) {	
		HW_USB_XHCI_Ring_insReq(dev->trRing[inEpId], req0);
		if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, inEpId + 1, 0, req0) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: get report failed, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
			while (1) IO_hlt(); 
		}
		HW_USB_HID_parseReport(repRaw, helper, rep);
		printk(WHITE, BLACK, "K: %d %d %d %d %d %d %d\r", 
				rep->items.keyboard.spK, 
				rep->items.keyboard.key[0], rep->items.keyboard.key[1], rep->items.keyboard.key[2],
				rep->items.keyboard.key[3], rep->items.keyboard.key[4], rep->items.keyboard.key[5]);
		Intr_SoftIrq_Timer_mdelay(inInterval);
	}
}

void HW_USB_HID_process(XHCI_Device *dev) {
	XHCI_Request *req0, *req1;

	printk(BLUE, BLACK, "dev %#018lx accept HID Driver\n", dev);

	// get the string descriptor of the configuration
	req1 = HW_USB_XHCI_allocReq(3);
	XHCI_StrDesc *strDesc = kmalloc(0xff, Slab_Flag_Private | Slab_Flag_Clear, NULL);
	HW_USB_XHCI_ctrlDataReq(req1, 
			HW_USB_XHCI_TRB_mkSetup(0x80, 0x06, 0x0300 | dev->devDesc->iProduct, 0x0409, 0xff),
			XHCI_TRB_Ctrl_Dir_In,
			strDesc, 0xff);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(WHITE, BLACK, "dev %#018lx: failed to get device string descriptor, code=%d\n", 
				dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	// convert the unicode string into ascii
	for (int i = 2; i < strDesc->hdr.len - 2; i += 2) strDesc->str[i >> 1] = strDesc->str[i], strDesc->str[i] = 0;
	printk(WHITE, BLACK, "dev %#018lx: configuration string descriptor: %s\n", dev, strDesc->str);
	kfree(req1, Slab_Flag_Private);
	
	USB_HID_Interface *inters = kmalloc(
			sizeof(USB_HID_Interface) * dev->cfgDesc[0]->bNumInter, 
			Slab_Flag_Private | Slab_Flag_Clear, NULL),
		*bstInter = NULL;

	int curInter = 0, curEp = 0, hasCtrlEp = 0;
	dev->inCtx->ctrl.addFlags = 1;
	// set up endpoints for all endpoints (doge)
	for (XHCI_DescHdr *hdr = &dev->cfgDesc[0]->hdr; hdr; hdr = HW_USB_XHCI_Desc_nxtCfgItem(dev->cfgDesc[0], hdr)) {
		switch (hdr->type) {
			case XHCI_Descriptor_Type_Cfg: {
				XHCI_CfgDesc *cfg = container(hdr, XHCI_CfgDesc, hdr);
				printk(WHITE, BLACK, "dev %#018lx: cfg %d: attr:%02x mxPw:%02x numInter:%d\n", dev, cfg->bCfgVal, cfg->bmAttr, cfg->bMxPw, cfg->bNumInter);
				continue;
			}
			case XHCI_Descriptor_Type_Inter: {
				XHCI_InterDesc *inter = container(hdr, XHCI_InterDesc, hdr);
				curInter = inter->bInterNum;
				curEp = 0;
				inters[curInter].desc = inter;
				inters[curInter].eps = kmalloc(sizeof(XHCI_EpDesc *) * sizeof(inter->bNumEp),
						Slab_Flag_Private | Slab_Flag_Clear, NULL);
				printk(WHITE, BLACK, "dev %#018lx: inter %d epNum:%d alter:%d class:%d subClass:%d proto:%d\n",
						dev, inter->bInterNum, inter->bNumEp, inter->bAlterSet, inter->bInterClass, inter->bInterSubClass, inter->bInterProto);
				hasCtrlEp = 0;
				break;
			}
			case XHCI_Descriptor_Type_HID:
				inters[curInter].hidDesc = container(hdr, USB_HidDesc, hdr);
				printk(WHITE, BLACK, "dev %#018lx: hidDesc: numDesc:%d descLen:%d descType:%d\n",
						dev, inters[curInter].hidDesc->bNumDesc, inters[curInter].hidDesc->wDescLen, inters[curInter].hidDesc->bDescType);
				break;
		}
		XHCI_EpDesc *epDesc = container(hdr, XHCI_EpDesc, hdr);
		int epId = HW_USB_XHCI_EpDesc_epId(epDesc),
			epType = (epDesc->bmAttr & 0x3) | ((epDesc->bEpAddr >> 5) & 0x4);

		printk(WHITE, BLACK, "dev %#018lx:\tepId:%d epType:%d mxPackSz:%d mxBurstSize:%d interval:%d\n", 
				dev, epId, epType, epDesc->wMxPackSz & 0x07ff, (epDesc->wMxPackSz & 0x1800) >> 11, epDesc->interval);
		
		// only focus on interrupt in endpoint
		if (epType == 0x7) {
			if (dev->inCtx->ctrl.addFlags & (1 << (epId + 1))) continue;
			XHCI_EpCtx *ep = &dev->inCtx->ep[epId];
			memset(ep, 0, sizeof(XHCI_EpCtx));

			HW_USB_XHCI_writeCtx(&dev->inCtx->slot, 0, XHCI_SlotCtx_ctxEntries, 
				max(HW_USB_XHCI_readCtx(&dev->inCtx->slot, 0, XHCI_SlotCtx_ctxEntries), epId + 1));

			HW_USB_XHCI_writeCtx(ep, 0, XHCI_EpCtx_interval,	epDesc->interval);
			HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_epType, 		epType);
			HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_CErr, 		3);
			HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxPackSize, 	epDesc->wMxPackSz & 0x07ff);
			HW_USB_XHCI_writeCtx(ep, 1, XHCI_EpCtx_mxBurstSize,	(epDesc->wMxPackSz & 0x1800) >> 11);

			dev->trRing[epId] = HW_USB_XHCI_allocRing(XHCI_Ring_maxSize);
			ep->deqPtr = DMAS_virt2Phys(dev->trRing[epId]->cur) | 1;

			HW_USB_XHCI_writeCtx(ep, 4, XHCI_EpCtx_aveTrbLen, (epType & 0x3 == 3 ? (1 << 10) : 8));

			HW_USB_XHCI_EpCtx_writeMxESITPay(ep, 
				HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxPackSize) * (HW_USB_XHCI_readCtx(ep, 1, XHCI_EpCtx_mxBurstSize) + 1));

			dev->inCtx->ctrl.addFlags |= (1u << (epId + 1));
			inters[curInter].eps[curEp++] = epDesc;
		}
	}
	// modify the endpoint using "Configure Endpoint Command"
	req0 = HW_USB_XHCI_allocReq(1);
	HW_USB_XHCI_TRB_setData(&req0->trb[0], DMAS_virt2Phys(dev->inCtx));
	HW_USB_XHCI_TRB_setSlot(&req0->trb[0], dev->slotId);
	HW_USB_XHCI_TRB_setType(&req0->trb[0], XHCI_TRB_Type_CfgEp);

	HW_USB_XHCI_Ring_insReq(dev->host->cmdRing, req0);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, 0, 0, 0, req0) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to configure endpoint(s), code=%d\n", 
			dev, HW_USB_XHCI_TRB_getCmplCode(&req0->res));
		while (1) IO_hlt();
	}
	printk(BLUE, BLACK, "dev %#018lx: configure endpoint(s)\n", dev);

	// set SET_CONFIGURATION request to device
	req1 = HW_USB_XHCI_allocReq(2);
	HW_USB_XHCI_ctrlReq(req1, HW_USB_XHCI_TRB_mkSetup(0x00, 0x09, dev->cfgDesc[0]->bCfgVal, 0, 0), XHCI_TRB_Ctrl_Dir_Out);
	HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
	if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "dev %#018lx: failed to set configuration, code=%d\n", dev, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
		while (1) IO_hlt();
	}
	printk(BLUE, BLACK, "dev %#018lx: set configuration successfully cfgVal:%d\n", dev, dev->cfgDesc[0]->bCfgVal);

	// get all the report descriptor and generate parse helper for each them
	// and then set SET_PROTOCOL request to use report descriptor
	for (int i = 0; i < dev->cfgDesc[0]->bNumInter; i++) {
		USB_HID_ParseHelper *helper = HW_USB_HID_mkParseHelper(dev, inters[i].desc, inters[i].hidDesc);
		inters[i].helper = helper;
		if (inters[i].desc->bInterSubClass == 0x01) {
			HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x21, 0x0b, 0x0001, inters[i].desc->bInterNum, 0));
			HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
			if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
				printk(RED, BLACK, "dev %#018lx: failed to set protocol for interface %d, code=%d\n", 
						dev, inters[i].desc->bInterNum, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
				while (1) IO_hlt();
			}
		}
		// set SET_IDLE
		HW_USB_XHCI_TRB_setData(&req1->trb[0], HW_USB_XHCI_TRB_mkSetup(0x21, 0x0a, 0x0000, inters[i].desc->bInterNum, 0));
		HW_USB_XHCI_Ring_insReq(dev->trRing[0], req1);
		if (HW_USB_XHCI_Req_ringDbWait(dev->host, dev->slotId, 1, 0, req1) != XHCI_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "dev %#018lx: failed to set idle for interface %d, code=%d\n",
					dev, inters[i].desc->bInterNum, HW_USB_XHCI_TRB_getCmplCode(&req1->res));
			while (1) IO_hlt();
		}
	}

	kfree(req0, Slab_Flag_Private);
	kfree(req1, Slab_Flag_Private);

	for (int i = 0; i < dev->cfgDesc[0]->bNumInter; i++) {
		switch (inters[i].helper->type) {
			case USB_HID_ReportHelper_Type_Mouse :
			case USB_HID_ReportHelper_Type_Keyboard :
				bstInter = &inters[i];
				break;
		}
		if (bstInter) break;
	}
	if (!bstInter) {
		printk(RED, BLACK, "dev %#018lx: unsupported HID device\n", dev);
		while (1) IO_hlt();
	}
	// for unused endpoint, also place a normal TRB.
	for (int i = 0; i < dev->cfgDesc[0]->bNumInter; i++) {
		if (bstInter == &inters[i]) continue;
		for (int j = 0; j < bstInter->desc->bNumEp; j++) {
			if (!inters[i].eps[j]) continue;
			int epId = HW_USB_XHCI_EpDesc_epId(inters[i].eps[j]);
			if (!(epId & 1) && epId > 0) {
				XHCI_Request *req = HW_USB_XHCI_allocReq(1);
				u64 *repRaw = kmalloc(inters[i].helper->inSz / 8, Slab_Flag_Private, NULL);
				HW_USB_XHCI_TRB_setData(&req->trb[0], DMAS_virt2Phys(repRaw));
				HW_USB_XHCI_TRB_setStatus(&req->trb[0], HW_USB_XHCI_TRB_mkStatus(inters[i].helper->inSz / 8, 0, 0));
				HW_USB_XHCI_TRB_setType(&req->trb[0], XHCI_TRB_Type_Normal);
				HW_USB_XHCI_TRB_setCtrlBit(&req->trb[0], XHCI_TRB_Ctrl_ioc);
				HW_USB_XHCI_Ring_insReq(dev->trRing[epId], req);
				HW_USB_XHCI_writeDbReg(dev->host, dev->slotId, epId + 1, 0);
			}
		}
	}
	// get the input endpoint id and output endpoint id
	int inEpId = 0, outEpId = -1, inInterval = 0;
	for (int i = 0; i < bstInter->desc->bNumEp; i++) {
		if (!bstInter->eps[i]) continue;
		int epId = HW_USB_XHCI_EpDesc_epId(bstInter->eps[i]);
		if (epId & 1) outEpId = epId;
		else inEpId = epId, inInterval = bstInter->eps[i]->interval;
	}
	printk(WHITE, BLACK, "dev %#018lx: inEpId:%d outEpId:%d inInterval:%d\n", dev, inEpId, outEpId, inInterval);
	switch (bstInter->helper->type) {
		case USB_HID_ReportHelper_Type_Keyboard:
			HW_USB_HID_processKeyboard(dev, bstInter->desc, bstInter->helper, inEpId, outEpId, inInterval);
			break;
		case USB_HID_ReportHelper_Type_Mouse:
			HW_USB_HID_processMouse(dev, bstInter->desc, bstInter->helper, inEpId, outEpId, inInterval);
			break;
	}
} 