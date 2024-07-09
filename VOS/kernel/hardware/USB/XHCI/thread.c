#include "inner.h"
#include "ringop.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"


typedef struct EnableSlotInfo {
	// the parent device information, set to NULL if attached to the host
	Device *parent;
	u8 spd, port;
} EnableSlotInfo;

static int mxPktSize(int spd) {
	switch (spd) {
		case 4: return 512; // super speed
		case 3: case 1: case 0: return 64; // full speed and high speed
		case 2: return 8;
	}
	return -1;
}

static void _addReq(USB_XHCIController *ctrl, USB_XHCIReq *req) {
	SpinLock_lock(&ctrl->witQueLock);
	List_insBefore(&req->listEle, &ctrl->witReqList);
	SpinLock_unlock(&ctrl->witQueLock);
}

static void _handler_setAddr(USB_XHCIController *ctrl, USB_XHCIReq *req, void *arg) {
	printk(YELLOW, BLACK, "XHCI: %#018lx: handler_setAddr(): req:%#018lx ", ctrl, req);
	int code = req->eve.dw[2] >> 24;
	if (code == HW_USB_XHCI_TRB_Completion_Success) {
		printk(GREEN, BLACK, "success\n");
	} else printk(RED, BLACK, "failed.(code:%d)\n", code);

	USB_XHCI_Device *dev = ctrl->devices[req->slot] = kmalloc(sizeof(USB_XHCI_Device), 0);
	dev->ctrl = ctrl;
	dev->ctx = ctrl->devCtx[req->slot];
	dev->enableEp = 3;
	
	USB_XHCI_InputCtrlContext *inCtx = arg;
	USB_XHCI_DeviceSlotContext *slotCtx = (USB_XHCI_DeviceSlotContext *)(inCtx + 1);
	USB_XHCI_EndpointContext *ep0 = (USB_XHCI_EndpointContext *)(slotCtx + 1);
	printk(WHITE, BLACK, "\tdev:%#018lx inCtx:%#018lx\n", dev, inCtx);
	dev->transferRing = kmalloc(sizeof(USB_XHCI_GenerTRB *) * 32, 0);
	dev->inqPtr = kmalloc(sizeof(USB_XHCI_GenerTRB *) * 32, 0);
	dev->cycFlags = kmalloc(sizeof(u8) * 32, 0);
	dev->transferRing[0] = dev->inqPtr[0] = DMAS_phys2Virt(ep0->dw2_3.trDeqPtr & ~0x1ul);
	dev->cycFlags[0] = 1;

	kfree(inCtx);
}

static void _handler_enblSlot(USB_XHCIController *ctrl, USB_XHCIReq *req, void *arg) {
	printk(YELLOW, BLACK, "XHCI: %#018lx: handler_enableSlot(): ", ctrl);
	int code = req->eve.dw[2] >> 24, slotId = ((USB_XHCI_CompletionTRB *)&req->eve)->dw3.ctx.slotId;
	if (code == HW_USB_XHCI_TRB_Completion_Success)
		printk(GREEN, BLACK, "successful.slotId:%d\n", slotId);
	else {
		printk(RED, BLACK, "failed\n");
		return ;
	}

	EnableSlotInfo *info = (EnableSlotInfo *)arg;

	// for USB2.0 devices, reset the port to enable it
	if (info->spd < 4) {
		ctrl->ports[info->port].regs->statusCtrl |= (1 << 4);
		for (i32 remain = 30; remain >= 0; remain--) {
			if (ctrl->ports[info->port].regs->statusCtrl & (1 << 21)) break;
			Intr_SoftIrq_Timer_mdelay(1);
		}
		if (!(ctrl->ports[info->port].regs->statusCtrl & (1 << 21)) || (ctrl->ports[info->port].regs->statusCtrl & (1 << 4))) {
			printk(RED, BLACK, "\tfail to reset USB 2.0 port.\n");
			return ;
		}
	}

	// create the input control context and add the request for setting address
	memset(req, 0, sizeof(req));

	req->req.dw3.ctx.trbType = HW_USB_TrbType_SetAddrCmd;
	req->req.dw3.raw |= ((u32)slotId) << 24;
	req->slot = slotId;

	{
		u64 ctxSz = CSZ(ctrl) ? 64 : 32;
		USB_XHCI_InputCtrlContext *inCtx = kmalloc(ctxSz * 32, 0);
		USB_XHCI_DeviceSlotContext *slotCtx = (USB_XHCI_DeviceSlotContext *)((u64)inCtx + ctxSz);
		USB_XHCI_EndpointContext *ep0Ctx = (USB_XHCI_EndpointContext *)((u64)slotCtx + ctxSz);

		memset(inCtx, 0, ctxSz * 32);

		inCtx->addFlags |= 3;

		// setting the basic information
		slotCtx->dw0.ctx.ctxEntries = 1;
		slotCtx->dw0.ctx.speed = info->spd;
		slotCtx->dw1.ctx.rootHubPort = info->port + 1;
		slotCtx->dw2.ctx.intTarget = slotId % maxIntrs(ctrl);

		printk(WHITE, BLACK, "\tslotCtx: speed:%d rootHubPort:%d intTarget:%d\n", slotCtx->dw0.ctx.speed, slotCtx->dw1.ctx.rootHubPort, slotCtx->dw2.ctx.intTarget);
		
		ep0Ctx->dw0.ctx.lsa = 1;
		ep0Ctx->dw0.ctx.interval = 0;
		ep0Ctx->dw1.ctx.errCnt = 3;
		ep0Ctx->dw1.ctx.epType = 4;
		ep0Ctx->dw1.ctx.mxPktSize = mxPktSize(info->spd);
		ep0Ctx->dw2_3.trDeqPtr = DMAS_virt2Phys(HW_USB_XHCI_allocTransferRing(ctrl, NULL, NULL)) | 1;

		ep0Ctx->dw4.ctx.avgTRBLen = 8;

		printk(WHITE, BLACK, "\tep0Ctx->dw1.mxPktSize=%d transfer ring:%#018lx ", ep0Ctx->dw1.ctx.mxPktSize, ep0Ctx->dw2_3.trDeqPtr);

		*(u64 *)&req->req.dw[0] = DMAS_virt2Phys(inCtx);
		req->arg = inCtx;

		printk(WHITE, BLACK, "inCtx:%#018lx\n", DMAS_virt2Phys(inCtx));

		IO_mfence();
	}

	req->handler = _handler_setAddr;

	kfree(info);

	_addReq(ctrl, req);
}

static void _portChgEvent(USB_XHCIController *ctrl, u32 port) {
	if (!(ctrl->ports[port].regs->statusCtrl & 1)) {
		printk(WHITE, BLACK, "XHCI: %#018lx: Port %d disconnected.\n", ctrl, port);
		if (ctrl->ports[port].dev != NULL)
			ctrl->ports[port].dev->uninstall(ctrl->ports[port].dev);
		ctrl->ports[port].dev = NULL;
		return ;
	}
	u8 spdId = (ctrl->ports[port].regs->statusCtrl >> 10) & ((1 << 4) - 1);
	printk(WHITE, BLACK, "XHCI: %#018lx: Port %d connect, speed %d\t", ctrl, port, spdId);
	printk(WHITE, BLACK, "\n");

	// initialize the request block
	USB_XHCIReq *req = (USB_XHCIReq *)kmalloc(sizeof(USB_XHCIReq), 0);
	memset(req, 0, sizeof(USB_XHCIReq));
	req->req.dw3.ctx.trbType = HW_USB_TrbType_EnblSlotCmd;

	// set the handler
	req->handler = _handler_enblSlot;
	{
		EnableSlotInfo *info = (EnableSlotInfo *)kmalloc(sizeof(EnableSlotInfo), 0);
		info->parent = NULL;
		info->port = port;
		info->spd = spdId;
		req->arg = info;
	}
	req->flag |= HW_USB_XHCIReq_Flag_isCommand;
	ctrl->ports[port].dev = NULL;
	_addReq(ctrl, req);
}

static inline int _getCmdPos(USB_XHCIController *ctrl, USB_XHCI_GenerTRB *cmd) {
	return (int)((u64)cmd - (u64)ctrl->cmdRing) / sizeof(USB_XHCI_GenerTRB);
}
u64 HW_USB_XHCI_thread(u64 (*_)(u64), u64 ctrlAddr) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	USB_XHCIController *ctrl = (USB_XHCIController *)ctrlAddr;
	int firPeriod = 1;
	printk(WHITE, BLACK, "HW_USB_XHCI_thread(): %#018lx\n", ctrl);
	while (1) {
		ctrl->opRegs->usbStatus = UsbState_EveIntr | UsbState_PortChange;
		for (int i = 0; i < maxPorts(ctrl); i++) {
			USB_XHCI_Port *port = &ctrl->ports[i];
			if (port->regs->statusCtrl & Port_StatusCtrl_ConnectChange) {
				_setPortStsCtrl(&port->regs->statusCtrl, Port_StatusCtrl_ConnectChange | Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
				_portChgEvent(ctrl, i);
			}
		}
		SpinLock_lock(&ctrl->lock);
		
		for (int i = 0; i < maxIntrs(ctrl); i++) {
			USB_XHCI_GenerTRB intrTRB;
			if (!(ctrl->rtRegs->intrRegs[i].mgrRegs & 0x1)) continue;
			while (HW_USB_XHCI_getNextEveTRB(ctrl, i, &intrTRB)) {
				printk(YELLOW, BLACK, "XHCI: %#018lx: new Event TRB: pos:%#018lx ", ctrl, ctrl->eveRingFlag[i].pos - 1);
				printk(WHITE, BLACK, "type:%d datas:%#018lx\n", intrTRB.dw3.ctx.trbType, *(u64 *)intrTRB.dw);
				if (intrTRB.dw3.ctx.trbType == HW_USB_TrbType_CmdCompletionEve) {
					USB_XHCI_GenerTRB *cmd = DMAS_phys2Virt(*(u64 *)&intrTRB.dw[0]);
					int pos = _getCmdPos(ctrl, cmd);

					// set the flag of the command ring
					ctrl->cmdsFlag[pos] = 1;

					// handle the request
					USB_XHCIReq *req = ctrl->cmdSrc[pos];
					ctrl->cmdSrc[pos] = NULL;
					if (req) {
						req->flag &= ~HW_USB_XHCIReq_Flag_isInRing;
						// copy the information into the request and execute the handler
						memcpy(&intrTRB, &req->eve, sizeof(USB_XHCI_GenerTRB));
						req->flag |= HW_USB_XHCIReq_Flag_Completed;
						if (req->handler != NULL) req->handler(ctrl, req, req->arg);
					}
				}
			}
			ctrl->rtRegs->intrRegs[i].mgrRegs = (ctrl->rtRegs->intrRegs[i].mgrRegs & ~0x3ul) | 0x3;
		}
		SpinLock_unlock(&ctrl->lock);
		SpinLock_lock(&ctrl->witQueLock);
		int hasCmd = 0;
		
		for (List *reqList = ctrl->witReqList.next, *nxt; reqList != &ctrl->witReqList; reqList = nxt) {
			nxt = reqList->next;
			USB_XHCIReq *req = container(reqList, USB_XHCIReq, listEle);
			int inserted = 0;
			if (req->flag & HW_USB_XHCIReq_Flag_isCommand) {
				USB_XHCI_GenerTRB *cmd = HW_USB_XHCI_getNextCmdTRB(ctrl);
				printk(WHITE, BLACK, "XHCI: %#018lx: try to push %#018lx into command ring ", ctrl, req);
				// the command ring is full
				if (cmd == NULL) { printk(RED, BLACK, "->command ring is full.\n"); continue; }
				memcpy(&req->req, cmd, sizeof(USB_XHCI_GenerTRB));
				hasCmd = inserted = 1;
				cmd->dw3.ctx.cycle = ctrl->cmdRingFlag.cycleBit;
				int pos = _getCmdPos(ctrl, cmd);
				ctrl->cmdSrc[pos] = req;
				ctrl->cmdSrc[pos]->flag |= HW_USB_XHCIReq_Flag_isInRing;
				ctrl->cmdSrc[pos]->flag &= ~HW_USB_XHCIReq_Flag_Completed;
				List_del(reqList);
				printk(GREEN, BLACK, "->done addr:%#018lx\n", cmd);
			} else {
				// is a transfer TRB
				
			}
		}
		SpinLock_unlock(&ctrl->witQueLock);
		if (hasCmd) {
			_writeDoorbell(ctrl, 0, 0);
		}
		firPeriod = 0;
	}
}