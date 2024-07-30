#include "inner.h"

SpinLock HW_USB_XHCI_DrvListLock;
List HW_USB_XHCI_DrvList;

void HW_USB_XHCI_insReqBlk(USB_XHCIController *ctrl, USB_XHCIReqBlock *reqs) {
	SpinLock_lock(&ctrl->witQueLock);
	List_insBefore(&reqs->listEle, &ctrl->witReqList);
	SpinLock_unlock(&ctrl->witQueLock);
}

int HW_USB_XHCI_waitRely(USB_XHCIController *ctrl, USB_XHCIReqBlock *reqs) {
	while (1) {
		SpinLock_lock(&ctrl->lock);
		u8 flag = reqs->flags;
		SpinLock_unlock(&ctrl->lock);
		if (flag & HW_USB_XHCIReq_Flag_replied) break;
		IO_hlt();
	}
	return 1;
}

USB_XHCIReqBlock *HW_USB_XHCI_mkCmdBlk(int trbType, u64 slot, u64 arg) {
	USB_XHCIReqBlock *req = kmalloc(sizeof(USB_XHCIReqBlock) + sizeof(USB_XHCI_GenerTRB), 0);
	memset(req, 0, sizeof(USB_XHCIReqBlock) + sizeof(USB_XHCI_GenerTRB));
	req->flags |= HW_USB_XHCIReq_Flag_isCommand;
	req->reqCnt = 1;
	List_init(&req->listEle);
	{
		USB_XHCI_GenerTRB *cmd = &req->reqs[0];
		*(u64 *)cmd->dw = arg;
		cmd->dw[2] |= slot << 24;
		cmd->dw3.ctx.trbType = trbType;
	}
	return req;
}

USB_XHCIReqBlock *HW_USB_XHCI_mkGetDescBlk(u64 slot, u64 descType, u64 idx, u64 wIdx, u64 len, void *buf) {
	USB_XHCIReqBlock *req = kmalloc(sizeof(USB_XHCIReqBlock) + sizeof(USB_XHCI_GenerTRB) * 5, 0);
	memset(req, 0, sizeof(USB_XHCIReqBlock) + 5 * sizeof(USB_XHCI_GenerTRB));
	req->reqCnt = 5;
	req->slot = slot;
	{
		USB_XHCI_SetupTRB *setup = (USB_XHCI_SetupTRB *)&req->reqs[0];
		setup->dw0.ctx.bmReqType = 0x80;
		setup->dw0.ctx.bReq = 6;
		setup->dw0.ctx.wVal = (descType << 8) | (idx);
		setup->dw1.ctx.wIndex = 0;
		setup->dw1.ctx.wLen = len;

		setup->dw2.ctx.trbLen = 8;
		setup->dw3.ctx.idt = 1;
		setup->dw3.ctx.trbType = HW_USB_TrbType_SetupStage;
		setup->dw3.ctx.tfType = 3;
	}
	{
		USB_XHCI_DataTRB *data = (USB_XHCI_DataTRB *)&req->reqs[1];
		data->dw0_1.dtBuf = DMAS_virt2Phys(buf);
		data->dw2.ctx.trbLen = len;
		data->dw3.ctx.evalNxtTRB = 1;
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_DataStage;
		data->dw3.ctx.direct = 1;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[2];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(64);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw3.ctx.ioc = 0;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
	{
		USB_XHCI_StatusTRB *data = (USB_XHCI_StatusTRB *)&req->reqs[3];
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_StatusStage;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[4];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(64);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw3.ctx.ioc = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
	return req;
}

void HW_USB_XHCI_freeReqBlk(USB_XHCIReqBlock *reqs) {
	for (int i = 0; i < reqs->reqCnt; i++) {
		if (reqs->reqs[i].dw3.ctx.trbType != HW_USB_TrbType_EventData) continue;
		USB_XHCI_EventDataBuffer *buf = container(DMAS_phys2Virt(*(u64 *)&reqs->reqs[i].dw[0]), USB_XHCI_EventDataBuffer, dt);
		kfree(buf, 0);
	}
	kfree(reqs, 0);
}

void HW_USB_XHCI_addDriver(USB_XHCI_Driver *drv) {
	SpinLock_lock(&HW_USB_XHCI_DrvListLock);
	List_init(&drv->listEle);
	List_insBefore(&drv->listEle, &HW_USB_XHCI_DrvList);
	SpinLock_unlock(&HW_USB_XHCI_DrvListLock);
}

void HW_USB_XHCI_delDriver(USB_XHCI_Driver *drv) {
	SpinLock_lock(&HW_USB_XHCI_DrvListLock);
	List_del(&drv->listEle);
	SpinLock_unlock(&HW_USB_XHCI_DrvListLock);
}

USB_XHCI_Driver *HW_USB_XHCI_getDriver(USB_XHCI_Device *dev) {
	USB_XHCI_Driver *bst = NULL;
	int bstSts = HW_USB_XHCI_DriverCheck_Unmatched, sts;
	SpinLock_lock(&HW_USB_XHCI_DrvListLock);
	for (List *drvList = HW_USB_XHCI_DrvList.next; drvList != &HW_USB_XHCI_DrvList; drvList = drvList->next) {
		USB_XHCI_Driver *drv = container(drvList, USB_XHCI_Driver, listEle);
		if ((sts = drv->chk(dev)) > bstSts) bstSts = sts, bst = drv;
	}
	SpinLock_unlock(&HW_USB_XHCI_DrvListLock);
	return bst;
}
