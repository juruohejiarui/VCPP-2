#include "inner.h"
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

void _ack_disableSlot(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev) {
	HW_USB_XHCI_normalAck(ctrl, req, dev);
	printk(ORANGE, BLACK, "XHCI: %#018lx: ack of disbale slot : ", ctrl);
	printk(WHITE, BLACK, "slot:%d ", (req->reqs[0].dw3.raw >> 24) - 1);
	
}

void _recycUnconfigDev(USB_XHCI_Device *dev, int inDevThread) {
	printk(WHITE, BLACK, "_recycleUnconfigDev: rflags:%#018lx\n", IO_getRflags());
	if (dev->slot != -1) {
		// disable the slot if this device
		USB_XHCI_ReqBlock *req = HW_USB_XHCI_mkCmdBlk(HW_USB_TrbType_DisableSlotCmd, dev->slot, 0, 0, 0);
		if (inDevThread) {
			HW_USB_XHCI_insBlk(dev->ctrl, req);
			HW_USB_XHCI_waitReply(dev->ctrl, req);
			kfree(req, Slab_kmalloc_arg_Private);
		} else {
			req->ack = (USB_XHCI_ReqAck)_ack_disableSlot;
			req->arg = dev;
			HW_USB_XHCI_insBlk(dev->ctrl, req);
		}
	}
	printk(WHITE, BLACK, "success to disable slot\n");
	for (int i = 0; i < 31; i++) {
		if (dev->transRing[i] != NULL) HW_USB_XHCI_free(dev->ctrl, dev->transRing[i]);
		if (dev->transSrc[i] != NULL)HW_USB_XHCI_free(dev->ctrl, dev->transSrc[i]);
	}
	kfree(dev->ctx, 0);
}

static void _ack_getDesc(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev) {
	HW_USB_XHCI_normalAck(ctrl, req, dev);
	printk(ORANGE, BLACK, "XHCI: %#018lx: ack of get descriptor: ");
	int code = req->res.dw[2] >> 24;
	if (code != 1) {
		printk(RED, BLACK, "fail(code:%d)\n", code);
		return ;
	}
	printk(GREEN, BLACK, "success\n");

	kfree(req, Slab_kmalloc_arg_Private);

	// free the request block
	// create a new thread to manage this device
	TaskStruct *devTsk = Task_createTask(HW_USB_XHCI_devThread, NULL, (u64)dev, Task_Flag_Inner | Task_Flag_Kernel);
	dev->drvTask = devTsk;
}

static void _ack_addrDev(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev) {
	HW_USB_XHCI_normalAck(ctrl, req, dev);
	printk(ORANGE, BLACK, "XHCI: %#018lx: ack of address device: ");
	int code = req->res.dw[2] >> 24;
	if (code != 1) {
		printk(RED, BLACK, "fail(code:%d)\n", code);
		_recycUnconfigDev(dev, 0);
		kfree(req, 0);
		return ;
	}
	printk(GREEN, BLACK, "success\n");

	int slot = (req->res.dw3.raw >> 24) - 1;
	ctrl->devices[slot] = dev;

	HW_USB_XHCI_freeReqBlk(req);

	dev->desc = kmalloc(sizeof(USB_XHCI_DevDesc), 0, NULL);
	req = HW_USB_XHCI_mkGetDescBlk(slot, HW_USB_XHCI_DescType_Device, 0, 0, sizeof(USB_XHCI_DevDesc), dev->desc);
	dev->roctx = DMAS_phys2Virt(ctrl->devCtx[req->slot + 1]);

	req->ack = (USB_XHCI_ReqAck)_ack_getDesc;
	req->arg = dev;

	HW_USB_XHCI_insBlk(ctrl, req);
}

static void _ack_enblSlot(USB_XHCIController *ctrl, USB_XHCI_ReqBlock *req, USB_XHCI_Device *dev) {
	HW_USB_XHCI_normalAck(ctrl, req, dev);
	printk(ORANGE, BLACK, "XHCI: %#018lx: ack of enable Slot: ");
	{
		int code = req->res.dw[2] >> 24;
		if (code != 1) {
			printk(RED, BLACK, "fail(code:%d\n)", code);
			_recycUnconfigDev(dev, 0);
			kfree(req, 0);
			return ;
		}
		printk(GREEN, BLACK, "success\n");
	}
	int slotId = req->res.dw3.raw >> 24;

	dev->slot = slotId - 1;
	
	// set the second request to address the device
	// we should first reset the port
	{
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

	ctx->epCtx[0].dw1.ctx.mxPktSize = mxPktSize(ctx->slotCtx.dw0.ctx.speed);
	ctx->epCtx[0].dw1.ctx.errCnt = 3;
	ctx->epCtx[0].dw1.ctx.epType = 4;

	// allocate a transfer ring and set the pointers
	dev->transRing[0] = HW_USB_XHCI_allocTransferRing(ctrl, NULL, NULL);
	dev->transInqPtr[0] = dev->transRing[0];
	dev->transSrc[0] = HW_USB_XHCI_alloc(ctrl, HW_USB_XHCI_RingEntryNum * sizeof(USB_XHCI_ReqBlock *));
	dev->transCycFlags[0] = 1;
	ctx->epCtx[0].dw2_3.trDeqPtr = 0x1 | DMAS_virt2Phys(dev->transRing[0]);

	ctx->epCtx[0].dw4.ctx.avgTRBLen = 8;

	memset(req->reqs, 0, sizeof(USB_XHCI_GenerTRB));

	*(u64 *)&req->reqs->dw[0] = DMAS_virt2Phys(ctx);
	req->reqs->dw3.ctx.trbType = HW_USB_TrbType_SetAddrCmd;
	req->reqs->dw3.raw |= (slotId << 24);

	req->ack = (USB_XHCI_ReqAck)_ack_addrDev;

	HW_USB_XHCI_insBlk(ctrl, req);
}

/// @brief the ack of port connection change
/// @param ctrl the xhci controller
/// @param portId the 0-based port index
static void _portChgEvent(USB_XHCIController *ctrl, int portId) {
	if (!(ctrl->ports[portId].regs->statusCtrl & 0x1)) {
		printk(ORANGE, BLACK, "XHCI: %#018lx: port %d disconnected\n", ctrl, portId);
		if (ctrl->ports[portId].dev != NULL) {
			u32 slot = ctrl->ports[portId].dev->slot;
			if (ctrl->ports[portId].dev->drvTask != NULL) Task_setSignal(ctrl->ports[portId].dev->drvTask, Task_Signal_Int);
			else _recycUnconfigDev(ctrl->ports[portId].dev, 0);
			if (slot != -1) ctrl->devices[slot] = NULL;
			ctrl->ports[portId].dev = NULL;
		}
		return ;
	}

	printk(ORANGE, BLACK, "XHCI: %#018lx: port %d connected ", ctrl, portId);
	// start to setup up the device of this port
	// roadmap: 1. enable a slot for it, 2. do address operation to setup context 3. get descriptor
	USB_XHCI_Device *dev = kmalloc(sizeof(USB_XHCI_Device), 0, NULL);
	memset(dev, 0, sizeof(USB_XHCI_Device));

	// set some basic info
	dev->ctrl = ctrl;
	dev->slot = -1;

	dev->ctx = kmalloc(sizeof(USB_XHCI_InputContext), 0, NULL);
	memset(dev->ctx, 0, sizeof(USB_XHCI_InputContext));
	dev->ctx->slotCtx.dw0.ctx.speed = (ctrl->ports[portId].regs->statusCtrl >> 10 & 0xful);
	dev->ctx->slotCtx.dw1.ctx.rootHubPort = portId + 1;
	dev->ctx->slotCtx.dw2.ctx.intTarget = 0;

	printk(WHITE, BLACK, "speed:%d\n", dev->ctx->slotCtx.dw0.ctx.speed);

	USB_XHCI_ReqBlock *reqBlk = kmalloc(sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_ReqBlock), 0, NULL);
	memset(reqBlk, 0, sizeof(USB_XHCI_ReqBlock) + sizeof(USB_XHCI_ReqBlock));
	// first requst block is for enabling slot
	reqBlk->reqCnt = 1;
	reqBlk->reqs[0].dw3.ctx.trbType = HW_USB_TrbType_EnblSlotCmd;

	reqBlk->flags = HW_USB_XHCIReq_Flag_isCommand;

	reqBlk->arg = dev;
	reqBlk->ack = (USB_XHCI_ReqAck)_ack_enblSlot;

	ctrl->ports[portId].dev = dev;

	HW_USB_XHCI_insBlk(ctrl, reqBlk); 
}

static void _handleWitQue(USB_XHCIController *ctrl) {
	SpinLock_lock(&ctrl->witQueLock);
	static u64 tmpList[256], tmpListLen;
	for (List *reqList = ctrl->witReqList.next, *nxt; reqList != &ctrl->witReqList; reqList = nxt) {
		nxt = reqList->next;
		USB_XHCI_ReqBlock *reqBlk = container(reqList, USB_XHCI_ReqBlock, listEle);
		int slotId, epId, enough = 1;
		tmpListLen = 0;
		if (reqBlk->flags & HW_USB_XHCIReq_Flag_isCommand) {
			// get enough spare TRB
			for (int i = 0; i < reqBlk->reqCnt; i++) {
				tmpList[tmpListLen] = ctrl->cmdRingFlag.cycleBit;
				USB_XHCI_GenerTRB *trb = HW_USB_XHCI_getNextCmdTRB(ctrl);
				if (trb == NULL) { enough = 0; break; }
				tmpList[tmpListLen++] |= (u64)trb;
				if (trb->dw3.ctx.trbType == HW_USB_TrbType_Link) {
					tmpList[tmpListLen - 1] |= 0x2;
					i--;
					continue;
				}
			}

			if (!enough) continue;
			
			reqBlk->target = kmalloc(reqBlk->reqCnt * sizeof(USB_XHCI_ReqBlock **), 0, NULL);
			for (int reqP = 0, listP = 0; listP < tmpListLen; listP++) {
				USB_XHCI_GenerTRB *trb = (void *)(tmpList[listP] & ~0x3ul);
				if (tmpList[listP] & 0x2) {
					trb->dw3.ctx.cycle = tmpList[listP] & 1;
					continue;
				}
				int pos = HW_USB_getRingPos(trb);
				reqBlk->target[reqP] = &ctrl->cmdSrc[pos];
				ctrl->cmdSrc[pos] = reqBlk;
				memcpy(&reqBlk->reqs[reqP], trb, sizeof(USB_XHCI_GenerTRB));
				trb->dw3.ctx.cycle = tmpList[listP] & 1;
				reqP++;
			}

			reqBlk->flags &= ~HW_USB_XHCIReq_Flag_replied;
			List_del(reqList);
			slotId = 0, epId = 0;
		} else { // is a transfer request block
			USB_XHCI_Device *dev = ctrl->devices[reqBlk->slot];
			if (dev == NULL) continue; 

			// save the state for restoring
			USB_XHCI_GenerTRB *_lstPtr = dev->transInqPtr[reqBlk->endpoint];
			u8 _lstFlag = dev->transCycFlags[reqBlk->endpoint];

			// get enough idle TRBs
			for (int i = 0; i < reqBlk->reqCnt; i++) {
				tmpList[tmpListLen] = dev->transCycFlags[reqBlk->endpoint];
				USB_XHCI_GenerTRB *trb = HW_USB_XHCI_getNextTransferTRB(ctrl, reqBlk->slot, reqBlk->endpoint);
				if (trb == NULL) { enough = 0; break; }
				tmpList[tmpListLen++] |= (u64)trb;
				if (trb->dw3.ctx.trbType == HW_USB_TrbType_Link) {
					tmpList[tmpListLen - 1] |= 0x2;
					i--;
					continue;
				}
			}

			// check if it failed to get enough idle TRBs
			if (!enough) {
				dev->transInqPtr[reqBlk->endpoint] = _lstPtr;
				dev->transCycFlags[reqBlk->endpoint] = _lstFlag;
				continue;
			}

			reqBlk->target = kmalloc(reqBlk->reqCnt * sizeof(USB_XHCI_ReqBlock **), 0, NULL);
			// copy the TRBs in request block into the transfer ring
			for (int reqP = 0, listP = 0; listP < tmpListLen; listP++) {
				USB_XHCI_GenerTRB *trb = (void *)(tmpList[listP] & ~0x3ul);
				if (tmpList[listP] & 0x2) {
					trb->dw3.ctx.cycle = tmpList[listP] & 1;
					continue;
				}
				int pos = HW_USB_getRingPos(trb);
				memcpy(&reqBlk->reqs[reqP], trb, sizeof(USB_XHCI_GenerTRB));
				trb->dw3.ctx.cycle = tmpList[listP] & 1;

				// set the trb pointer of the buffer structure
				if (trb->dw3.ctx.trbType == HW_USB_TrbType_EventData)
					container(DMAS_phys2Virt(*(u64 *)trb->dw), USB_XHCI_EventDataBuffer, dt)->trb = (USB_XHCI_NormalTRB *)trb;

				// set cycle bit and the pointer in source list
				dev->transSrc[reqBlk->endpoint][pos] = reqBlk;
				reqBlk->target[reqP] = &dev->transSrc[reqBlk->endpoint][pos];
				reqP++;
			}
			reqBlk->flags &= ~HW_USB_XHCIReq_Flag_replied;
			List_del(reqList);
			slotId = reqBlk->slot + 1, epId = reqBlk->endpoint + 1;
		}
		_writeDoorbell(ctrl, slotId, epId);
		break;
	}
	SpinLock_unlock(&ctrl->witQueLock);
}
u64 HW_USB_XHCI_mainThread(u64 (*_)(u64), u64 ctrlAddr) {
	Task_kernelEntryHeader();
	USB_XHCIController *ctrl = (USB_XHCIController *)ctrlAddr;
	int firPeriod = 1;
	printk(WHITE, BLACK, "HW_USB_XHCI_mainThread(): %#018lx\n", ctrl);
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
				switch (intrTRB.dw3.ctx.trbType) {
					case HW_USB_TrbType_CmdCompletionEve: {
						USB_XHCI_GenerTRB *cmd = DMAS_phys2Virt(*(u64 *)&intrTRB.dw[0]);
						int pos = HW_USB_getRingPos(cmd);

						// handle the request
						USB_XHCI_ReqBlock *reqBlk = ctrl->cmdSrc[pos];
						if (reqBlk) {							
							reqBlk->flags |= HW_USB_XHCIReq_Flag_replied;
							// copy the information into the request and execute the ack
							memcpy(&intrTRB, &reqBlk->res, sizeof(USB_XHCI_GenerTRB));
							
							if (reqBlk->ack != NULL) reqBlk->ack(ctrl, reqBlk, reqBlk->arg);
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

						USB_XHCI_Device *dev = ctrl->devices[slot - 1];
						
						USB_XHCI_ReqBlock *reqBlk = dev->transSrc[ep][pos];
						if (reqBlk) {
							// clear the transSrc entries of this request block
							memcpy(&intrTRB, &reqBlk->res, sizeof(USB_XHCI_GenerTRB));
							if (reqBlk->ack != NULL) {
								reqBlk->flags |= HW_USB_XHCIReq_Flag_replied;
								reqBlk->ack(ctrl, reqBlk, reqBlk->arg);
							}
						}
						break;
					}
				}
			}
			ctrl->rtRegs->intrRegs[i].mgrRegs = (ctrl->rtRegs->intrRegs[i].mgrRegs & ~0x3ul) | 0x3;
		}
		SpinLock_unlock(&ctrl->lock);

		_handleWitQue(ctrl);
		firPeriod = 0;
	}
	Task_kernelThreadExit(0);
}

void _signalHandler_Int(u64 signal, u64 devAddr) {
	if (!devAddr) { printk(WHITE, BLACK, "signal handler for INT: unable to kill this thread...\n"); return ; }
	USB_XHCI_Device *dev = (USB_XHCI_Device *)devAddr;
	printk(WHITE, BLACK, "signal handler for INT of dev %#018lx thread pid:%ld\n", dev, Task_current->pid);
	if (dev->cfgDesc) {
		for (int i = 0; i < dev->desc->numConfig; i++)
			if (dev->cfgDesc[i] != NULL) kfree(dev->cfgDesc[i], 0);
		kfree(dev->cfgDesc, 0);
	}
	_recycUnconfigDev(dev, 1);
	Task_kernelThreadExit(0);
}

u64 HW_USB_XHCI_devThread(u64 (*_)(u64), u64 devAddr) {
	Task_kernelEntryHeader();
	Task_setSignalHandler(Task_Signal_Int, (Task_SignalHandler)_signalHandler_Int, devAddr);

	USB_XHCI_Device *dev = (USB_XHCI_Device *)devAddr;

	printk(WHITE, BLACK, "XHCI: dev %#018lx: mkPkt=%d\n", dev, dev->desc->mxPkt0);
	// get the string descriptor for further management
	dev->cfgDesc = kmalloc(sizeof(USB_XHCI_ConfigDesc *) * sizeof(dev->desc->numConfig), Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < dev->desc->numConfig; i++) {
		dev->cfgDesc[i] = kmalloc(0xff, 0, NULL);
		USB_XHCI_ReqBlock *reqs = HW_USB_XHCI_mkGetDescBlk(dev->slot, HW_USB_XHCI_DescType_Config, i, 0, 0xff, dev->cfgDesc[i]);
		HW_USB_XHCI_insBlk(dev->ctrl, reqs); 
		HW_USB_XHCI_waitReply(dev->ctrl, reqs);
		kfree(reqs, Slab_kmalloc_arg_Private);
	}

	// search for driver for this device
	while (1) {
		USB_XHCI_Driver *drv = HW_USB_XHCI_getDriver(dev);
		if (drv != NULL) {
			int sts = drv->loader(dev);
			if (sts == HW_USB_XHCI_DriverCheck_Success) {
				printk(WHITE, BLACK, "XHCI: %#018lx: manage device %#018lx with driver \"%s\"\n", dev->ctrl, dev, drv->name);
				break;
			}
		}
		Intr_SoftIrq_Timer_mdelay(1000);
	}
	Task_kernelThreadExit(0);
}