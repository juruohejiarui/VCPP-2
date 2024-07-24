#include "inner.h"
#include "ringop.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"

static int mxPktSize(int spd) {
	switch (spd) {
		case 4: return 512; // super speed
		case 3: case 1: return 64;	// full speed and high speed
		case 2: case 0: return 8;	// low speed and USB 1.1 / USB 1.0
		default: return 512;
	}
	return -1;
}

static void _addReq(USB_XHCIController *ctrl, USB_XHCIReqBlock *req) {
	SpinLock_lock(&ctrl->witQueLock);
	List_insBefore(&req->listEle, &ctrl->witReqList);
	SpinLock_unlock(&ctrl->witQueLock);
}

static int _resetPort(USB_XHCIController *ctrl, int portId) {
	u32 *ptr = &ctrl->ports[portId].regs->statusCtrl;
	_setPortStsCtrl(ptr, Port_StatusCtrl_Reset | Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
	for (int i = 0; i < 30; i++) {
		if (!(*ptr & Port_StatusCtrl_Reserved) && (*ptr & (1 << 21))) break;
		Intr_SoftIrq_Timer_mdelay(1);
	}
	if (*ptr & Port_StatusCtrl_Reset || !(*ptr & (1 << 21))) return -1;
	return 0;
}

static void _handler_getDesc(USB_XHCIController *ctrl, USB_XHCIReqBlock *req, USB_XHCI_Device *dev) {
	printk(WHITE, BLACK, "XHCI: %#018lx: handler of get descriptor: ");
	int code = req->res.dw[2] >> 24;
	if (code != 1) {
		printk(RED, BLACK, "fail(code:%d)\n", code);
		return ;
	}
	printk(GREEN, BLACK, "success\n");
	printk(WHITE, BLACK, "\tbcdUSB: %1x:%02x class:subClass: %02x:%02x Proto: %02x mxPkt0: %02x\n",
		dev->desc[3], dev->desc[2], dev->desc[4], dev->desc[5], dev->desc[6], dev->desc[7]);
	printk(WHITE, BLACK, "\tvendor: %04x product: %04x bcdDev: %04x\n",
		*(u16 *)&dev->desc[8], *(u16 *)&dev->desc[10], *(u16 *)&dev->desc[12]);
	printk(WHITE, BLACK, "\tiManufacturer: %02x iProduct: %02x iSerialNumber: %02x bNumConfig: %02x\n",
		dev->desc[14], dev->desc[15], dev->desc[16], dev->desc[17]);
}

static void _handler_addrDev(USB_XHCIController *ctrl, USB_XHCIReqBlock *req, USB_XHCI_Device *dev) {
	printk(WHITE, BLACK, "XHCI: %#018lx: handler of address device: ");
	int code = req->res.dw[2] >> 24;
	if (code != 1) {
		printk(RED, BLACK, "fail(code:%d)\n", code);
		kfree(dev->ctx), HW_USB_XHCI_free(ctrl, dev->transRing), HW_USB_XHCI_free(ctrl, dev->transSrc);
		kfree(dev);
		kfree(req->reqs);
		kfree(req);
		return ;
	}
	printk(GREEN, BLACK, "success\n");

	req->slot = (req->res.dw3.raw >> 24) - 1;

	ctrl->devices[req->slot] = dev;
	dev->roctx = DMAS_phys2Virt(ctrl->devCtx[req->slot + 1]);

	// set the transfer package
	memset(req->reqs, 0, sizeof(USB_XHCI_GenerTRB) * 5);
	req->reqCnt = 5;
	req->endpoint = 0;
	req->flags &= ~HW_USB_XHCIReq_Flag_isCommand;
	{
		USB_XHCI_setupTRB *setup = (USB_XHCI_setupTRB *)&req->reqs[0];
		setup->dw0.ctx.bmReqType = 0x80;
		setup->dw0.ctx.bReq = 6;
		setup->dw0.ctx.wVal = 0x0100;
		setup->dw1.ctx.wIndex = 0;
		setup->dw1.ctx.wLen = 0x12;

		setup->dw2.ctx.trbLen = 8;
		setup->dw3.ctx.idt = 1;
		setup->dw3.ctx.trbType = HW_USB_TrbType_SetupStage;
		setup->dw3.ctx.tfType = 3;
	}
	{
		USB_XHCI_DataTRB *data = (USB_XHCI_DataTRB *)&req->reqs[1];
		dev->desc = kmalloc(64, 0);
		memset(dev->desc, 0, 64);
		data->dw0_1.dtBuf = DMAS_virt2Phys(dev->desc);
		data->dw2.ctx.trbLen = 0x12;
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

	req->handler = (USB_XHCIReqHandler)_handler_getDesc;

	_addReq(ctrl, req);
}

static void _handler_enblSlot(USB_XHCIController *ctrl, USB_XHCIReqBlock *req, USB_XHCI_Device *dev) {
	printk(WHITE, BLACK, "XHCI: %#018lx: handler of enable Slot: ");
	{
		int code = req->res.dw[2] >> 24;
		if (code != 1) {
			printk(RED, BLACK, "fail(code:%d\n)", code);
			kfree(dev->ctx), kfree(dev); kfree(req->reqs); kfree(req);
			return ;
		}
		printk(GREEN, BLACK, "success\n");
	}
	int slotId = req->res.dw3.raw >> 24;
	printk(WHITE, BLACK, "\tslot:%d for port:%d, speed:%d ", 
		slotId, dev->ctx->slotCtx.dw1.ctx.rootHubPort,
		dev->ctx->slotCtx.dw0.ctx.speed);
	
	// set the second request to address the device
	// for a USB 2.0 device, we should first reset the port
	if (dev->ctx->slotCtx.dw0.ctx.speed < 4) {
		int port = dev->ctx->slotCtx.dw1.ctx.rootHubPort - 1;
		if (_resetPort(ctrl, port) == -1) { printk(RED, BLACK, "\tfailed to reset port\n"); return ; }
		// wait and clear the port change bit
		for (int i = 0; i < 30; i++) {
			if (ctrl->ports[port].regs->statusCtrl & Port_StatusCtrl_ConnectChange) {
				_setPortStsCtrl(&ctrl->ports[port].regs->statusCtrl,
					Port_StatusCtrl_ConnectChange | Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
				break;
			}
			Intr_SoftIrq_Timer_mdelay(1);
		}
	}
	// adjust the content of input context
	USB_XHCI_InputContext *ctx = dev->ctx;

	ctx->inCtx.addFlags = 0x3;
	
	ctx->slotCtx.dw0.ctx.ctxEntries = 1;

	ctx->epCtx[0].dw0.ctx.lsa = 1;
	ctx->epCtx[0].dw1.ctx.mxPktSize = mxPktSize(ctx->slotCtx.dw0.ctx.speed);
	ctx->epCtx[0].dw1.ctx.errCnt = 3;
	ctx->epCtx[0].dw1.ctx.epType = 4;

	// allocate a transfer ring and set the pointers
	dev->transRing[0] = HW_USB_XHCI_allocTransferRing(ctrl, NULL, NULL);
	dev->transInqPtr[0] = dev->transRing[0];
	dev->transSrc[0] = HW_USB_XHCI_alloc(ctrl, HW_USB_XHCI_RingEntryNum * sizeof(USB_XHCIReqBlock *));
	dev->transCycFlags[0] = 1;
	ctx->epCtx[0].dw2_3.trDeqPtr = 0x1 | DMAS_virt2Phys(dev->transRing[0]);
	printk(WHITE, BLACK, "ep0->trDepPtr=%#018lx\n", ctx->epCtx[0].dw2_3.trDeqPtr);

	ctx->epCtx[0].dw4.ctx.avgTRBLen = 8;

	memset(req->reqs, 0, sizeof(USB_XHCI_GenerTRB));

	*(u64 *)&req->reqs->dw[0] = DMAS_virt2Phys(ctx);
	req->reqs->dw3.ctx.trbType = HW_USB_TrbType_SetAddrCmd;
	req->reqs->dw3.raw |= (slotId << 24);

	req->handler = (USB_XHCIReqHandler)_handler_addrDev;

	_addReq(ctrl, req);
}

/// @brief the handler of port connection change
/// @param ctrl the xhci controller
/// @param portId the 0-based port index
static void _portChgEvent(USB_XHCIController *ctrl, int portId) {
	if (!(ctrl->ports[portId].regs->statusCtrl & 0x1)) {
		printk(WHITE, BLACK, "XHCI: %#018lx: port %d disconnected\n", ctrl, portId);
		return ;
	}
	printk(WHITE, BLACK, "XHCI: %#018lx: port %d connected ", ctrl, portId);
	// start to setup up the device of this port
	// roadmap: 1. enable a slot for it, 2. do address operation to setup context 3. get descriptor
	USB_XHCI_Device *dev = kmalloc(sizeof(USB_XHCI_Device), 0);
	memset(dev, 0, sizeof(USB_XHCI_Device));

	// set some basic info
	dev->ctrl = ctrl;

	dev->ctx = kmalloc(sizeof(USB_XHCI_InputContext), 0);
	memset(dev->ctx, 0, sizeof(USB_XHCI_InputContext));
	dev->ctx->slotCtx.dw0.ctx.speed = (ctrl->ports[portId].regs->statusCtrl >> 10 & 0xful);
	dev->ctx->slotCtx.dw1.ctx.rootHubPort = portId + 1;
	dev->ctx->slotCtx.dw2.ctx.intTarget = 0;

	printk(WHITE, BLACK, "speed:%d\n", dev->ctx->slotCtx.dw0.ctx.speed);

	USB_XHCIReqBlock *reqBlk = kmalloc(sizeof(USB_XHCIReqBlock) + 4 * sizeof(USB_XHCIReqBlock), 0);
	memset(reqBlk, 0, sizeof(USB_XHCIReqBlock) + 4 * sizeof(USB_XHCIReqBlock));

	// first requst block is for enabling slot
	reqBlk->reqCnt = 1;
	reqBlk->reqs[0].dw3.ctx.trbType = HW_USB_TrbType_EnblSlotCmd;

	reqBlk->flags = HW_USB_XHCIReq_Flag_isCommand;

	reqBlk->arg = dev;
	reqBlk->handler = (USB_XHCIReqHandler)_handler_enblSlot;

	_addReq(ctrl, reqBlk);
}
u64 HW_USB_XHCI_thread(u64 (*_)(u64), u64 ctrlAddr) {
	static u64 tmpList[256];
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	USB_XHCIController *ctrl = (USB_XHCIController *)ctrlAddr;
	int firPeriod = 1;
	printk(WHITE, BLACK, "HW_USB_XHCI_thread(): %#018lx\n", ctrl);
	while (1) {
		// clean the event interrupter bbit and the port change bit
		ctrl->opRegs->usbStatus = UsbState_EveIntr | UsbState_PortChange;
		for (int i = 0; i < maxPorts(ctrl); i++) {
			USB_XHCI_Port *port = &ctrl->ports[i];
			if (port->regs->statusCtrl & Port_StatusCtrl_ConnectChange) {
				_setPortStsCtrl(&port->regs->statusCtrl, Port_StatusCtrl_ConnectChange | Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
				_portChgEvent(ctrl, i);
			}
		}
		SpinLock_lock(&ctrl->lock);
		
		// scan each event ring
		for (int i = 0; i < maxIntrs(ctrl); i++) {
			USB_XHCI_GenerTRB intrTRB;
			if (!(ctrl->rtRegs->intrRegs[i].mgrRegs & 0x1)) continue;
			while (HW_USB_XHCI_getNextEveTRB(ctrl, i, &intrTRB)) {
				printk(YELLOW, BLACK, "XHCI: %#018lx: new Event TRB: pos:%04d ", ctrl, ctrl->eveRingFlag[i].pos - 1);
				printk(WHITE, BLACK, "type:%d datas:%#018lx\n", intrTRB.dw3.ctx.trbType, *(u64 *)intrTRB.dw);
				switch (intrTRB.dw3.ctx.trbType) {
					case HW_USB_TrbType_CmdCompletionEve: {
						USB_XHCI_GenerTRB *cmd = DMAS_phys2Virt(*(u64 *)&intrTRB.dw[0]);
						int pos = HW_USB_getRingPos(cmd);

						// handle the request
						USB_XHCIReqBlock *reqBlk = ctrl->cmdSrc[pos];
						ctrl->cmdSrc[pos] = NULL;
						if (reqBlk) {
							// copy the information into the request and execute the handler
							memcpy(&intrTRB, &reqBlk->res, sizeof(USB_XHCI_GenerTRB));
							
							if (reqBlk->handler != NULL) reqBlk->handler(ctrl, reqBlk, reqBlk->arg);
						}
						break;
					}
					case HW_USB_TrbType_TransferEve: {
						USB_XHCI_NormalTRB *data = NULL;

						// if it is from a event data, then should use the buffer structure to get the event data TRB
						int ed = (intrTRB.dw3.raw >> 2) & 1;
						if (ed) data = container(DMAS_phys2Virt(*(u64 *)&intrTRB.dw[0]), USB_XHCI_EventDataBuffer, dt)->trb;
						else data = DMAS_phys2Virt(*(u64 *)&intrTRB.dw[0]);

						int pos = HW_USB_getRingPos((USB_XHCI_GenerTRB *)data), slot = intrTRB.dw3.raw >> 24, ep = ((intrTRB.dw3.raw >> 16) & 0x1f) - 1;

						printk(WHITE, BLACK, "from %d-%d ptr:%#018lx pos:%d code:%d trLen:%d ed:%d\n",
							slot, ep, data, pos, (intrTRB.dw[2] >> 24), intrTRB.dw[2] & ((1 << 24) - 1), ed);
						USB_XHCI_Device *dev = ctrl->devices[slot - 1];

						
						USB_XHCIReqBlock *reqBlk = dev->transSrc[ep][pos];
						dev->transSrc[ep][pos] = NULL;
						if (reqBlk) {
							memcpy(&intrTRB, &reqBlk->res, sizeof(USB_XHCI_GenerTRB));
							if (reqBlk->handler != NULL) reqBlk->handler(ctrl, reqBlk, reqBlk->arg);
						}
						break;
					}
				}
			}
			ctrl->rtRegs->intrRegs[i].mgrRegs = (ctrl->rtRegs->intrRegs[i].mgrRegs & ~0x3ul) | 0x3;
		}
		SpinLock_unlock(&ctrl->lock);
		SpinLock_lock(&ctrl->witQueLock);
		
		for (List *reqList = ctrl->witReqList.next, *nxt; reqList != &ctrl->witReqList; reqList = nxt) {
			nxt = reqList->next;
			USB_XHCIReqBlock *reqBlk = container(reqList, USB_XHCIReqBlock, listEle);
			int inserted = 0;
			if (reqBlk->flags & HW_USB_XHCIReq_Flag_isCommand) {
				printk(WHITE, BLACK, "XHCI: %#018lx: insert request block %#018lx into command ring ", ctrl, reqBlk);
				int enough = 1;
				for (int i = 0; i < reqBlk->reqCnt; i++) {
					tmpList[i] = ctrl->cmdRingFlag.cycleBit | (u64)HW_USB_XHCI_getNextCmdTRB(ctrl);
					if ((void *)(tmpList[i] & ~0x1ul) == NULL) { enough = 0; break; }
				}
				if (!enough) { printk(RED, BLACK, "->fail, not enough idle TRB\n"); continue; }
				for (int i = 0; i < reqBlk->reqCnt; i++) {
					USB_XHCI_GenerTRB *trb = (void *)(tmpList[i] & ~0x1ul);

					int pos = HW_USB_getRingPos(trb);
					memcpy(&reqBlk->reqs[i], trb, sizeof(USB_XHCI_GenerTRB));
					trb->dw3.ctx.cycle = tmpList[i] & 1;
					ctrl->cmdSrc[pos] = reqBlk;
				}
				List_del(reqList);
				printk(GREEN, BLACK, "->succes\n");
				_writeDoorbell(ctrl, 0, 0);
				break;
			} else { // is a transfer request block
				printk(WHITE, BLACK, "XHCI: %#018lx: try to push %#018lx into transfer ring %d-%d\n", ctrl, reqBlk, reqBlk->slot, reqBlk->endpoint);
				USB_XHCI_Device *dev = ctrl->devices[reqBlk->slot];
				if (dev == NULL) { printk(RED, BLACK, "->fail, the device does not exist.\n"); continue; }
				int remain = 0;

				USB_XHCI_DeviceSlotContext *slotCtx = DMAS_phys2Virt(ctrl->devCtx[reqBlk->slot + 1]);
				USB_XHCI_EndpointContext *epCtx = (USB_XHCI_EndpointContext *)((u64)slotCtx + sizeof(USB_XHCI_DeviceSlotContext)) + reqBlk->endpoint;

				// save the state for restoring
				USB_XHCI_GenerTRB *_lstPtr = dev->transInqPtr[reqBlk->endpoint];
				u8 _lstFlag = dev->transCycFlags[reqBlk->endpoint];

				printk(WHITE, BLACK, "\tslotState:%d epState:%d transInqPtr:%#018lx\t", slotCtx->dw3.ctx.slotState, epCtx->dw0.ctx.epState, _lstPtr);

				// get enough idle TRBs
				for (int i = 0; i < reqBlk->reqCnt; i++) {
					tmpList[i] = dev->transCycFlags[reqBlk->endpoint] | (u64)HW_USB_XHCI_getNextTransferTRB(ctrl, reqBlk->slot, reqBlk->endpoint);
					if ((void *)(tmpList[i] & ~0x1ul) == NULL) { remain = reqBlk->reqCnt - i; break; }
				}

				// check if it failed to get enough idle TRBs
				if (remain) {
					printk(RED, BLACK, "\t->fail, not enough idle TRB, remain %d trb(s)\n", remain);
					dev->transInqPtr[reqBlk->endpoint] = _lstPtr;
					dev->transCycFlags[reqBlk->endpoint] = _lstFlag;
					continue;
				}

				// copy the TRBs in request block into the transfer ring
				for (int i = 0; i < reqBlk->reqCnt; i++) {
					USB_XHCI_GenerTRB *trb = (void *)(tmpList[i] & ~0x1ul);
					int pos = HW_USB_getRingPos(trb);
					memcpy(&reqBlk->reqs[i], trb, sizeof(USB_XHCI_GenerTRB));

					// set the trb pointer of the buffer structure
					if (trb->dw3.ctx.trbType == HW_USB_TrbType_EventData)
						container(DMAS_phys2Virt(*(u64 *)trb->dw), USB_XHCI_EventDataBuffer, dt)->trb = (USB_XHCI_NormalTRB *)trb;

					// set cycle bit and the pointer in source list
					trb->dw3.ctx.cycle = tmpList[i] & 1;
					dev->transSrc[reqBlk->endpoint][pos] = reqBlk;
				}
				List_del(reqList);
				_writeDoorbell(ctrl, reqBlk->slot + 1, (reqBlk->endpoint + 1));
				printk(GREEN, BLACK, "->success\n");
				break;
			}
		}
		SpinLock_unlock(&ctrl->witQueLock);
		firPeriod = 0;
	}
}