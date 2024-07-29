#include "api.h"

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