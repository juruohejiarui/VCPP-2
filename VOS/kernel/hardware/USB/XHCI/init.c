#include "../XHCI.h"
#include "inner.h"
#include "ringop.h"
#include "../../../includes/interrupt.h"
#include "../../../includes/memory.h"
#include "../../../includes/log.h"

List HW_USB_XHCI_mgrList;

// allocate memory for the controller，use DMAS_virt2Phys to get the physical address
static void *_alloc(USB_XHCIController *ctrl, u64 size) {
	if (size == 0) return NULL;
	void *addr; Page *page;
	if (size >= Page_4KSize) {
		page = MM_Buddy_alloc(log2Ceil(size) - Page_4KShift, Page_Flag_Kernel | Page_Flag_Active);
		if (page == NULL) goto _alloc_Fail;
		addr = DMAS_phys2Virt(page->phyAddr);
	} else {
		addr = kmalloc(size, 0);
		if (addr == NULL) goto _alloc_Fail;
	}
	USB_XHCI_MemUsage *usage = (USB_XHCI_MemUsage *)kmalloc(sizeof(USB_XHCI_MemUsage), 0);
	List_init(&usage->listEle);
	List_insBefore(&usage->listEle, &ctrl->memList);
	if (size < Page_4KSize) usage->addr = (u64)addr;
	else usage->addr = ((u64)page) | 1;
	memset(addr, 0, size);
	return addr;
	_alloc_Fail:
	printk(RED, BLACK, "XHCI: alloc memory fail");
	printk(YELLOW, BLACK, "(ctrl:%#018lx,size:%#018lx)", ctrl, size);
	return NULL;
}

static void _free(USB_XHCIController *ctrl) {
	// free all pages
	USB_XHCI_MemUsage *usage;
	while (!List_isEmpty(&ctrl->memList)) {
		usage = container(ctrl->memList.next, USB_XHCI_MemUsage, listEle);
		List_del(&usage->listEle);
		if (usage->addr & 1) MM_Buddy_free((Page *)(usage->addr ^ 1));
		else kfree((void *)usage->addr);
		kfree(usage);
	}
}

USB_XHCI_ExtCapEntry *_getNextExtCap(USB_XHCI_ExtCapEntry *extCap) {
	if (extCap->nxtOff == 0) return NULL;
	return (USB_XHCI_ExtCapEntry *)((u64)extCap + extCap->nxtOff * 4);
}

/// @brief search the legacy support capability and set the os owned bit
/// @param ctrl the controller
/// @return if success return 1, else return 0
static int _getOwnership(USB_XHCIController *ctrl) {
	USB_XHCI_ExtCap_Legacy *legacy = NULL;
	for (USB_XHCI_ExtCapEntry *entry = ctrl->extCapHeader; entry != NULL; entry = _getNextExtCap(entry)) {
		if (entry->id != USB_XHCI_ExtCap_Id_Legacy) continue;
		legacy = (USB_XHCI_ExtCap_Legacy *)entry;
		break;
	}
	if (legacy == NULL) {
		printk(WHITE, BLACK, "XHCI: %#018lx: no legacy support.\n", ctrl);
		return 1;
	}
	u16 prevVal = legacy->data1;
	// bit 8 is the os owned bit
	legacy->data1 |= (1 << 8);
	for (int i = 0; i <= 10; i++) {
		if ((legacy->data1 & ((1 << 8) | (1 << 0))) == (1 << 8)) {
			printk(WHITE, BLACK, "XHCI: %#018lx: success to get ownership. (prevVal:%x)\n", ctrl, prevVal);
			return 1;
		} 
		Intr_SoftIrq_Timer_mdelay(2);
	}
	printk(GREEN, BLACK, "XHCI: %#018lx: failed to get ownership.\n", ctrl);
	return 0;
}

static int _stopController(USB_XHCIController *ctrl) {
	// set the run/stop bit to 0 -> stop the controller
	_clrCmdBit(ctrl, 0);
	// wait 30ms for the controller to stop
	u64 remain = 30;
	do {
		// wait 5ms
		Intr_SoftIrq_Timer_mdelay(5);
		// check the bit HCHalted of controller status
		if (_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_HCHalted)) break;
	} while (remain -= 5);
	if (!_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_HCHalted)) {
		printk(RED, BLACK, "XHCI: stop controller failed\n");
		ctrl->dev.free((Device *)ctrl), kfree(ctrl);
		return 0;
	}
	printk(WHITE, BLACK, "XHCI: %#018lx: success to stop controller.", ctrl);
	return 1;
}

static int _resetController(USB_XHCIController *ctrl) {
	 _setCmdBit(ctrl, HW_USB_XHCI_OpReg_Cmd_HCReset);
	 {
		u64 remain = 30;
		do {
			// wait 5ms
			Intr_SoftIrq_Timer_mdelay(5);
			// check the bit HCHalted of controller status
			if (!_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_CtrlNotReady) && !_rdCmdBit(ctrl, HW_USB_XHCI_OpReg_Cmd_HCReset))
				break;
		} while (remain -= 5);
		if (_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_CtrlNotReady) || _rdCmdBit(ctrl, HW_USB_XHCI_OpReg_Cmd_HCReset)) {
			printk(RED, BLACK, "XHCI: %#018lx: reset controller failed\n", ctrl);
			return 0;
		}
	}
	
	// check the default value
	if (ctrl->opRegs->usbCmd || ctrl->opRegs->devNotifCtrl
			 || ctrl->opRegs->cmdRingCtrl || ctrl->opRegs->devCtxBaseAddr || ctrl->opRegs->config) {
		printk(RED, BLACK, "XHCI: %#018lx: reset failed: default value check failed\n", ctrl);
		return 0;
	}
	printk(WHITE, BLACK, "XHCI: %#018lx: success to reset controller\n", ctrl);
	return 1;
}

static int _initPorts(USB_XHCIController *ctrl) {
	// allocate memory for each port
	ctrl->ports = (USB_XHCI_Port *)_alloc(ctrl, sizeof(USB_XHCI_Port) * maxPorts(ctrl));
	if (ctrl->ports == 0) {
		printk(RED, BLACK, "XHCI: %#018lx: allocate memory for ports failed\n", ctrl);
		return 0;
	}
	for (int i = 0; i < maxPorts(ctrl); i++) {
		ctrl->ports[i].regs = (USB_XHCI_PortRegs *)(ctrl->opRegs->portRegs + i);
		ctrl->ports[i].flags = 0;
		ctrl->ports[i].pair = NULL;
	}
	// set the flag isUSB3
	for (USB_XHCI_ExtCapEntry *entry = ctrl->extCapHeader; entry != NULL; entry = _getNextExtCap(entry)) {
		if (entry->id != USB_XHCI_ExtCap_Id_Protocol) continue;
		USB_XHCI_ExtCap_Protocol *protocol = container(entry, USB_XHCI_ExtCap_Protocol, extCap);
		printk(WHITE, BLACK, "XHCI: %#018lx: slotType[%d,%d]=%d\n", ctrl, protocol->portOff, protocol->portCnt + protocol->portOff - 1, protocol->slotType);
		for (int i = 0; i < protocol->portCnt; i++)
			ctrl->ports[protocol->portOff + i - 1].flags |= 
					(protocol->majorRev == 3 ? HW_USB_XHCI_Port_Flag_USB3 : 0) | HW_USB_XHCI_Port_Flag_Master,
			ctrl->ports[protocol->portOff + i - 1].offset = i,
			ctrl->ports[protocol->portOff + i - 1].slotType = protocol->slotType;
	}
	// set the flag isPaired
	printk(WHITE, BLACK, "XHCI: %#018lx: socket info: ", ctrl);
	for (int i = 0; i < maxPorts(ctrl); i++) {
		for (int j = i + 1; j < maxPorts(ctrl); j++) {
			if (ctrl->ports[i].offset != ctrl->ports[j].offset) continue;
			ctrl->ports[i].flags |= HW_USB_XHCI_Port_Flag_Paired;
			// clear the Master flag of the slave port
			if (HW_USB_XHCI_Port_isUSB3(&ctrl->ports[i])) ctrl->ports[j].flags ^= HW_USB_XHCI_Port_Flag_Master;
			else ctrl->ports[i].flags ^= HW_USB_XHCI_Port_Flag_Master;
			ctrl->ports[j].flags |= HW_USB_XHCI_Port_Flag_Paired;
			ctrl->ports[i].pair = &ctrl->ports[j];
			ctrl->ports[j].pair = &ctrl->ports[i];
			printk(WHITE, BLACK, "(%d,%d) ", i, j);
		}
	}
	printk(WHITE, BLACK, "\n");
	return 1;
}

static int _initMem(USB_XHCIController *ctrl) {
	// allocate the device context base address array
	void *addr = _alloc(ctrl, 2048);
	if (addr == NULL) return 0;
	ctrl->opRegs->devCtxBaseAddr = DMAS_virt2Phys(addr);
	ctrl->devCtx = addr;
	printk(WHITE, BLACK, "XHCI: %#018lx: devCtxBaseAddr:%#018lx\n", ctrl, ctrl->opRegs->devCtxBaseAddr);
	// allocate the Device Context Data Structure
	for (int i = 1; i < maxSlots(ctrl); i++) {
		addr = _alloc(ctrl, 
				((ctrl->capRegs->hccparam1 & 0x4) ? 64 * 32 : sizeof(USB_XHCI_DeviceSlotContext) + 31 * sizeof(USB_XHCI_EndpointContext)));
		if (addr == NULL) return 0;
		ctrl->devCtx[i] = (USB_XHCI_DeviceContext *)DMAS_virt2Phys(addr);
	}

	// allocate scratch buffer
	{
		int mxS = maxScratchBufs(ctrl); u64 pageSize = (ctrl->opRegs->pageSize & 0xfffful) << 12;
		if (mxS == 0) goto _allocScratchBuf_end;
		u64 *array = _alloc(ctrl, max(64, mxS * sizeof(u64)));
		printk(WHITE, BLACK, "XHCI: %#018lx: maxScratchBufs:%d array: %#018lx\n", ctrl, mxS, array);
		ctrl->devCtx[0] = (USB_XHCI_DeviceContext *)DMAS_virt2Phys(array);
		for (int i = 0; i < mxS; i++) {
			void *buf = _alloc(ctrl, pageSize);
			array[i] = (u64)DMAS_virt2Phys(buf);
		}
	}
	_allocScratchBuf_end:

	// allocate command ring of 64 KB
	{
		USB_XHCI_GenerTRB *cmdRing = _alloc(ctrl, Page_4KSize * 16), *lkTRB = cmdRing + HW_USB_XHCI_RingEntryNum - 1;
		ctrl->cmdRing = cmdRing;
		ctrl->cmdRingFlag.cycleBit = 1;
		ctrl->cmdRingFlag.segId = 0;
		ctrl->cmdRingFlag.pos = 0;
		printk(WHITE, BLACK, "XHCI: %#018lx: cmdRingCtrl:%#018lx\n", ctrl, cmdRing);
		ctrl->cmdsFlag = _alloc(ctrl, HW_USB_XHCI_RingEntryNum * sizeof(u64));
		for (int i = 0; i < HW_USB_XHCI_RingEntryNum; i++) ctrl->cmdsFlag[i] = 1;
		ctrl->cmdSrc = _alloc(ctrl, HW_USB_XHCI_RingEntryNum * sizeof(USB_XHCIReq *));

		// construct a link trb
		*((u64 *)&lkTRB->dw[0]) = DMAS_virt2Phys(cmdRing);
		lkTRB->dw3.ctx.trbType = HW_USB_TrbType_Link;
		lkTRB->dw3.raw |= 3; // Toggle Cycle | Cycle bit
	}
	// allocate event ring for each interrupter
	ctrl->eveRingSegTbls = _alloc(ctrl, max(64, sizeof(USB_XHCI_EveRingSegTblEntry *) * maxIntrs(ctrl)));
	for (int i = 0; i < maxIntrs(ctrl); i++) {
		// stop the interrupter
		ctrl->rtRegs->intrRegs[i].mgrRegs = 0x1;
		USB_XHCI_IntrRegs *regs = ctrl->rtRegs->intrRegs + i;
		// allocate 4 segments for one interrupter
		USB_XHCI_EveRingSegTblEntry *segTbl = _alloc(ctrl, max(64, sizeof(USB_XHCI_EveRingSegTblEntry) * HW_USB_XHCI_EveRingSegTblSize));
		ctrl->eveRingSegTbls[i] = segTbl;
		for (int tblId = 0; tblId < HW_USB_XHCI_EveRingSegTblSize; tblId++) {
			USB_XHCI_GenerTRB *eveRing = _alloc(ctrl, Page_4KSize * 16);
			ctrl->eveRingSegTbls[i][tblId].addr = (u64)DMAS_virt2Phys(eveRing);
			ctrl->eveRingSegTbls[i][tblId].size = HW_USB_XHCI_RingEntryNum;
		}
		ctrl->rtRegs->intrRegs[i].eveSegTblSize = (ctrl->rtRegs->intrRegs->eveSegTblSize & ~0xffff) | HW_USB_XHCI_EveRingSegTblSize;
		ctrl->rtRegs->intrRegs[i].eveSegTblAddr = DMAS_virt2Phys(segTbl) | (ctrl->rtRegs->intrRegs[i].eveSegTblAddr & 0x3f);
		ctrl->rtRegs->intrRegs[i].eveDeqPtr = 0x8 | segTbl[0].addr;
	}
	ctrl->eveRingFlag = _alloc(ctrl, sizeof(USB_XHCI_RingFlag) * maxIntrs(ctrl));
	for (int i = 0; i < maxIntrs(ctrl); i++) ctrl->eveRingFlag[i].cycleBit = 1;
}

int _restartController(USB_XHCIController *ctrl) {
	ctrl->opRegs->devNotifCtrl = (ctrl->opRegs->devNotifCtrl & ~0xffff) | (1 << 1);
	ctrl->opRegs->cmdRingCtrl = (ctrl->opRegs->cmdRingCtrl & ~(1ul << 1)) | (1ul << 1);
	ctrl->opRegs->usbStatus |= (1 << 2) | (1 << 3) | (1 << 4) | (1 << 10);
	ctrl->opRegs->usbCmd |= (1 << 0) | (1 << 2) | (1 << 3);
	for (int remain = 30; remain > 0; remain--) {
		Intr_SoftIrq_Timer_mdelay(2);
		if (ctrl->opRegs->usbStatus & 1) continue;
		break;
	}
	if ((ctrl->opRegs->usbStatus & 1) || ((ctrl->opRegs->usbCmd & 1) == 0)) {
		printk(RED, BLACK, "XHCI: %#018lx: restart failed\n", ctrl);
		return 0;
	}
	for (int i = 0; i < 1; i++) {
		ctrl->rtRegs->intrRegs[i].mgrRegs = 0x3 | (ctrl->rtRegs->intrRegs[i].mgrRegs & (~0x3));
	}
	ctrl->opRegs->cmdRingCtrl = DMAS_virt2Phys(ctrl->cmdRing) | 0x3;
	for (USB_XHCI_GenerTRB *eveTrb = HW_USB_XHCI_getNextEveTRB(ctrl, 0); eveTrb != NULL; eveTrb = HW_USB_XHCI_getNextEveTRB(ctrl, 0)) ;
	ctrl->opRegs->usbStatus = 0x8;

	for (int i = 0; i < maxPorts(ctrl); i++) {
		_setPortStsCtrl(&ctrl->ports[i].regs->statusCtrl, Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
	}
	return 1;
}

int HW_USB_XHCI_Init(PCIeConfig *xhci) {
    USB_XHCIController *ctrl = (USB_XHCIController *)kmalloc(sizeof(USB_XHCIController), 0);
	List_init(&ctrl->listEle), List_init(&ctrl->memList);

	// set the device struct
	ctrl->dev.install = NULL;
	ctrl->dev.uninstall = NULL;
	ctrl->dev.free = (void (*)(Device *))_free;

	List_init(&ctrl->witReqList);

	ctrl->config = xhci;
	u64 addr = (xhci->type.type0.bar[0] | (((u64)xhci->type.type0.bar[1]) << 32)) & ~0xffful;
	printk(WHITE, BLACK, "XHCI: %#018lx: Finish memory mapping\n", ctrl);
	// get the capability registers
	ctrl->capRegs = (USB_XHCI_CapRegs *)DMAS_phys2Virt(addr);
	// get the operational registers
	ctrl->opRegs = (USB_XHCI_OpRegs *)((u64)ctrl->capRegs + ctrl->capRegs->capLen);
	// get the runtime registers
	ctrl->rtRegs = (USB_XHCI_RuntimeRegs *)((u64)ctrl->capRegs + ctrl->capRegs->rtsoff);
	// get the doorbell registers
	ctrl->dbRegs = (USB_XHCI_DoorbellRegs *)((u64)ctrl->capRegs + ctrl->capRegs->dboff);
	// get extented capability registers
	ctrl->extCapHeader = (USB_XHCI_ExtCapEntry *)((u64)ctrl->capRegs + extCapPtr(ctrl) * 4);
	
	SpinLock_init(&ctrl->lock);
	SpinLock_init(&ctrl->witQueLock);
	printk(YELLOW, BLACK, "capReg:%#018lx opRegs:%#018lx rtRegs:%#018lx dbRegs:%#018lx extCap:%#018lx Slot:%d Intr:%d port:%d\n",
			ctrl->capRegs, ctrl->opRegs, ctrl->rtRegs, ctrl->dbRegs, ctrl->extCapHeader,
			maxSlots(ctrl), maxIntrs(ctrl), maxPorts(ctrl));

	int state;
	state = _stopController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// reset the controller
	state = _resetController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	state = _getOwnership(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// pair up the USB3 and USB2
	state = _initPorts(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// allocate memory for the controller
	state = _initMem(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	state = _restartController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	List_insBefore(&ctrl->listEle, &HW_USB_XHCI_mgrList);
	return 1;
}