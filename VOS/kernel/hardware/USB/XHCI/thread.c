#include "inner.h"
#include "ringop.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"


static void _handler_enblSlot(USB_XHCIController *ctrl, USB_XHCIReq *req, void *arg) {
	printk(YELLOW, BLACK, "XHCI: %#018lx: handler_enableSlot(): ", ctrl);
	printk(WHITE, BLACK, "reg->eve->slotId:%d\n", ((USB_XHCI_CompletionTRB *)&req->eve)->dw3.ctx.slotId);
	// load descriptor and allocate the management structure

}

static void _addReq(USB_XHCIController *ctrl, USB_XHCIReq *req) {
	SpinLock_lock(&ctrl->witQueLock);
	List_insBefore(&req->listEle, &ctrl->witReqList);
	SpinLock_unlock(&ctrl->witQueLock);
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
	if (spdId == 0) { printk(RED, BLACK, "Error...\n"); return ; }
	printk(WHITE, BLACK, "\n");

	// initialize the request block
	USB_XHCIReq *req = (USB_XHCIReq *)kmalloc(sizeof(USB_XHCIReq), 0);
	memset(req, 0, sizeof(USB_XHCIReq));
	req->handler = _handler_enblSlot;
	req->req.dw3.ctx.trbType = HW_USB_TrbType_EnblSlotCmd;
	req->arg = (void *)(u64)port;
	req->flag |= HW_USB_XHCIReq_Flag_isCommand;
	ctrl->ports[port].dev = NULL;
	List_init(&req->listEle);
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
			USB_XHCI_GenerTRB *intrTRB;
			if (!(ctrl->rtRegs->intrRegs[i].eveDeqPtr & 0x8)) continue;
			while ((intrTRB = HW_USB_XHCI_getNextEveTRB(ctrl, i)) != NULL) {
				printk(YELLOW, BLACK, "XHCI: %#018lx: new Event TRB: %#018lx ", ctrl, intrTRB);
				printk(WHITE, BLACK, "type:%d datas:%#018lx\n", intrTRB->dw3.ctx.trbType, *(u64 *)intrTRB->dw);
				if (intrTRB->dw3.ctx.trbType == HW_USB_TrbType_CmdCompletionEve) {
					USB_XHCI_GenerTRB *cmd = DMAS_phys2Virt(*(u64 *)&intrTRB->dw[0]);
					int pos = _getCmdPos(ctrl, cmd);

					// set the flag of the command ring
					ctrl->cmdsFlag[pos] = 1;

					// handle the request
					USB_XHCIReq *req = ctrl->cmdSrc[pos];
					req->flag &= ~HW_USB_XHCIReq_Flag_isInRing;
					// copy the information into the request and execute the handler
					memcpy(intrTRB, &req->eve, sizeof(USB_XHCI_GenerTRB));
					req->flag |= HW_USB_XHCIReq_Flag_Completed;
					if (req->handler != NULL) req->handler(ctrl, req, req->arg);
				}
			}
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
				printk(GREEN, BLACK, "->done\n");
			} else {
				// is a transfer TRB
			}
		}
		SpinLock_unlock(&ctrl->witQueLock);
		if (hasCmd) {
			ctrl->dbRegs->cmd = 0;
			IO_mfence();
		}
		firPeriod = 0;
	}
}