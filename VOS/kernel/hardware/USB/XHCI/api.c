#include "inner.h"
#include "../../../includes/log.h"

SpinLock HW_USB_XHCI_drvListLock;
List HW_USB_XHCI_drvList;

void HW_USB_XHCI_insBlk(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *reqs) {
	SpinLock_lock(&ctrl->witQueLock);
	List_insBefore(&reqs->listEle, &ctrl->witReqList);
	SpinLock_unlock(&ctrl->witQueLock);
}

int HW_USB_XHCI_waitReply(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *reqs) {
	while (1) {
		SpinLock_lock(&ctrl->lock);
		u8 flag = reqs->flags;
		SpinLock_unlock(&ctrl->lock);
		if (flag & HW_USB_XHCIReq_Flag_replied) break;
		IO_hlt();
	}
	return 1;
}

int HW_USB_XHCI_chkSucc(USB_XHCI_ReqBlock *reqs) {
	return reqs->res.dw3.ctx.trbType && (reqs->res.dw[2] >> 24) == 1;
}

void HW_USB_XHCI_normalAck(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev) {
	if (req->target) {
		for (int i = 0; i < req->reqCnt; i++) *req->target[i] = NULL;
		kfree(req->target, 0);
	}
	if (!HW_USB_XHCI_chkSucc(req)) printk(RED, BLACK, "reqs %#018lx failed. code=%d\n", req, req->res.dw[2] >> 24), req->flags |= HW_USB_XHCIReq_Flag_failed;
}

static void _setNormal(USB_XHCI_ReqBlock *req) {
	req->ack = (USB_XHCI_ReqAck)HW_USB_XHCI_normalAck;
}

USB_XHCI_ReqBlock *HW_USB_XHCI_mkCmdBlk(int trbType, u64 slot, u64 arg, u32 status, u32 flags) {
	USB_XHCI_ReqBlock *req = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB), Slab_kmalloc_arg_Private, (void *)HW_USB_XHCI_freeReqBlk);
	memset(req, 0, sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB));
	req->flags |= HW_USB_XHCIReq_Flag_isCommand;
	req->reqCnt = 1;
	_setNormal(req);
	{
		USB_XHCI_GenerTRB *cmd = &req->reqs[0];
		*(u64 *)cmd->dw = arg;
		cmd->dw[2] = status;
		cmd->dw3.raw |= ((slot + 1) << 24) | flags;
		cmd->dw3.ctx.trbType = trbType;
	}
	return req;
}

void _setSetup(USB_XHCI_SetupTRB *trb, u8 reqType, u8 req, u16 val, u16 idx, u16 len, u8 tfType) {
	trb->dw0.ctx.bmReqType = reqType;
	trb->dw0.ctx.bReq = req;
	trb->dw0.ctx.wVal = val;
	trb->dw1.ctx.wIndex = idx;
	trb->dw1.ctx.wLen = len;

	trb->dw2.ctx.trbLen = 8;
	trb->dw3.ctx.idt = 1;
	trb->dw3.ctx.trbType = HW_USB_TrbType_SetupStage;
	trb->dw3.ctx.tfType = tfType;
}

void _setDataStatus(USB_XHCI_ReqBlock *req, u32 direct, u32 idx, u64 len, void *buf) {
	{
		USB_XHCI_DataTRB *data = (USB_XHCI_DataTRB *)&req->reqs[idx];
		data->dw0_1.dtBuf = DMAS_virt2Phys(buf);
		data->dw2.ctx.trbLen = len;
		data->dw3.ctx.evalNxtTRB = 1;
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_DataStage;
		data->dw3.ctx.direct = direct;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[idx + 1];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(8);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw2.ctx.trbLen = 8;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
	{
		USB_XHCI_StatusTRB *data = (USB_XHCI_StatusTRB *)&req->reqs[idx + 2];
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_StatusStage;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[idx + 3];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(8);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw2.ctx.trbLen = 8;
		data->dw3.ctx.ioc = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
}

USB_XHCI_ReqBlock *HW_USB_XHCI_mkGetDescBlk(u64 slot, u64 descType, u64 idx, u64 wIdx, u64 len, void *buf) {
	USB_XHCI_ReqBlock *req = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB) * 5, Slab_kmalloc_arg_Private, (void *)HW_USB_XHCI_freeReqBlk);
	memset(req, 0, sizeof(USB_XHCI_ReqBlock) + 5 * sizeof(USB_XHCI_GenerTRB));
	req->reqCnt = 5;
	req->slot = slot;
	_setNormal(req);
	_setSetup((USB_XHCI_SetupTRB *)&req->reqs[0], 0x80, 6, (descType << 8) | idx, 0, len, 3);
	_setDataStatus(req, 1, 1, len, buf);
	return req;
}

USB_XHCI_ReqBlock *HW_USB_XHCI_mkSetCfgBlk(u64 slot, u64 cfgVal) {
	USB_XHCI_ReqBlock *req = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB) * 3, Slab_kmalloc_arg_Private, (void *)HW_USB_XHCI_freeReqBlk);
	memset(req, 0, sizeof(USB_XHCI_ReqBlock) + 3 * sizeof(USB_XHCI_GenerTRB));
	req->reqCnt = 3;
	req->slot = slot;
	_setNormal(req);
	_setSetup((USB_XHCI_SetupTRB *)&req->reqs[0], 0, 9, cfgVal, 0, 0, 0);
	{
		USB_XHCI_StatusTRB *data = (USB_XHCI_StatusTRB *)&req->reqs[1];
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_StatusStage;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[2];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(0xff);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw2.ctx.trbLen = 0xff;
		data->dw3.ctx.ioc = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
	return req;
}

USB_XHCI_ReqBlock *HW_USB_XHCI_mkSetIdleBlk(u64 slot, u32 reportId, u32 interval, u32 interfaceId) {
	USB_XHCI_ReqBlock *req = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB) * 3, Slab_kmalloc_arg_Private, (void *)HW_USB_XHCI_freeReqBlk);
	memset(req, 0, sizeof(USB_XHCI_ReqBlock) + 3 * sizeof(USB_XHCI_GenerTRB));
	req->reqCnt = 3;
	req->slot = slot;
	_setNormal(req);
	_setSetup((USB_XHCI_SetupTRB *)&req->reqs[0], 0x21, 0x0a, (interval << 8) | reportId, interfaceId, 0, Slab_kmalloc_arg_Private);
	{
		USB_XHCI_StatusTRB *data = (USB_XHCI_StatusTRB *)&req->reqs[1];
		data->dw3.ctx.chainBit = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_StatusStage;
	}
	{
		USB_XHCI_NormalTRB *data = (USB_XHCI_NormalTRB *)&req->reqs[2];
		USB_XHCI_EventDataBuffer *buf = HW_USB_XHCI_makeEveDataBuf(0xff);
		data->dw0_1.dtBufPtr = DMAS_virt2Phys(buf->dt);
		data->dw2.ctx.trbLen = 0xff;
		data->dw3.ctx.ioc = 1;
		data->dw3.ctx.trbType = HW_USB_TrbType_EventData;
	}
	return req;
}


USB_XHCI_ReqBlock *HW_USB_XHCI_mkGetReportBlk(u64 slot, u64 ep, u32 reportType, u32 reportId, u32 interfaceId, u32 len, void *buf) {
    USB_XHCI_ReqBlock *req = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_GenerTRB) * 5, 0, (void *)HW_USB_XHCI_freeReqBlk);
	memset(req, 0, sizeof(USB_XHCI_ReqBlock) + 5 * sizeof(USB_XHCI_GenerTRB));
	req->reqCnt = 5;
	req->slot = slot;
	req->endpoint = ep;
	_setNormal(req);
	_setSetup((USB_XHCI_SetupTRB *)&req->reqs[0], 0xa1, 0x01, (reportType << 8) | reportId, interfaceId, len, 3);
	_setDataStatus(req, 0, 1, len, buf);
	return req;
}
void HW_USB_XHCI_freeReqBlk(USB_XHCI_ReqBlock *reqs) {
    for (int i = 0; i < reqs->reqCnt; i++) {
		if (reqs->reqs[i].dw3.ctx.trbType != HW_USB_TrbType_EventData) continue;
		USB_XHCI_EventDataBuffer *buf = container(DMAS_phys2Virt(*(u64 *)&reqs->reqs[i].dw[0]), USB_XHCI_EventDataBuffer, dt);
		kfree(buf, 0);
	}
}

void HW_USB_XHCI_addDriver(USB_XHCI_Driver *drv) {
	SpinLock_lock(&HW_USB_XHCI_drvListLock);
	List_init(&drv->listEle);
	List_insBefore(&drv->listEle, &HW_USB_XHCI_drvList);
	SpinLock_unlock(&HW_USB_XHCI_drvListLock);
}

void HW_USB_XHCI_delDriver(USB_XHCI_Driver *drv) {
	SpinLock_lock(&HW_USB_XHCI_drvListLock);
	List_del(&drv->listEle);
	SpinLock_unlock(&HW_USB_XHCI_drvListLock);
}

USB_XHCI_Driver *HW_USB_XHCI_getDriver(USB_XHCI_Device *dev) {
	USB_XHCI_Driver *bst = NULL;
	int bstSts = HW_USB_XHCI_DriverCheck_Unmatched, sts;
	SpinLock_lock(&HW_USB_XHCI_drvListLock);
	for (List *drvList = HW_USB_XHCI_drvList.next; drvList != &HW_USB_XHCI_drvList; drvList = drvList->next) {
		USB_XHCI_Driver *drv = container(drvList, USB_XHCI_Driver, listEle);
		if ((sts = drv->chk(dev)) > bstSts) bstSts = sts, bst = drv;
	}
	SpinLock_unlock(&HW_USB_XHCI_drvListLock);
	return bst;
}

USB_XHCI_DescHeader *HW_USB_XHCI_getNxtDesc(USB_XHCI_ConfigDesc *cfg, USB_XHCI_DescHeader *hdr) {
	if (hdr == NULL) return (USB_XHCI_DescHeader *)((u64)cfg + cfg->header.len);

	USB_XHCI_DescHeader *nxt = (USB_XHCI_DescHeader *)((u64)hdr + hdr->len);
	return (u64)nxt - (u64)cfg < cfg->totLen ? nxt : NULL;
}
