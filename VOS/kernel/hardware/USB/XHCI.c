#include "XHCI.h"
#include "../../includes/interrupt.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"

#define maxSlots(ctrl) ((ctrl->capRegs->hcsParams1) & 0xff)
#define maxIntrs(ctrl) ((ctrl->capRegs->hcsParams1 >> 8) & 0x7ff)
#define maxPorts(ctrl) ((ctrl->capRegs->hcsParams1 >> 24) & 0xff)
#define extCapPtr(ctrl) ((ctrl->capRegs->hccparam1 >> 16) & 0xffff)
#define ist(ctrl) (ctrl->capRegs->hcsParams2 & 0x7)
#define maxEveRingSegTbl(ctrl) ((ctrl->capRegs->hcsParams2 & 0xf) >> 4);

#define _HCCP1_AC64		0
#define _HCPP1_PWRCTRL	3
static inline u64 maxScratchBufs(USB_XHCIController *ctrl) {
	u64 higt = ctrl->capRegs->hcsParams2 >> 21 & 0x1f,
		low = ctrl->capRegs->hcsParams2 >> 27 & 0x1f;
	return (higt << 5) | low;
}
#define ac64(ctrl) (ctrl->capRegs->hcsParams3 & 0x1)

// allocate memory for the controller，use DMAS_virt2Phys to get the physical address
static void *_alloc(USB_XHCIController *ctrl, u64 size) {
	void *addr; Page *page;
	if (size > Page_4KSize / 2) {
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
	if (size <= Page_4KSize / 2) usage->addr = (u64)addr;
	else usage->addr = ((u64)page) | 1;
	memset(addr, 0, size);
	return addr;
	_alloc_Fail:
	printk(RED, BLACK, "XHCI: alloc memory fail");
	printk(YELLOW, BLACK, "(ctrl:%#018lx,size%#018lx)", ctrl, size);
	return NULL;
}

List HW_USB_XHCI_mgrList;

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

static inline void _setCmdBit(USB_XHCIController *ctrl, u32 bit) {
	ctrl->opRegs->usbCmd = (ctrl->opRegs->usbCmd & (~(1u << bit))) | (1u << bit);
}
static inline void _clrCmdBit(USB_XHCIController *ctrl, u32 bit) {
	ctrl->opRegs->usbCmd &= ~(1u << bit);
}

static inline int _rdCmdBit(USB_XHCIController *ctrl, u32 bit) {
	return (ctrl->opRegs->usbCmd >> bit) & 1;
}

static inline int _rdStsBit(USB_XHCIController *ctrl, u32 bit) {
	return (ctrl->opRegs->usbStatus >> bit) & 1;
}

USB_XHCI_ExtCapEntry *_getNextExtCap(USB_XHCI_ExtCapEntry *extCap) {
	if (extCap->nxtOff == 0) return NULL;
	return (USB_XHCI_ExtCapEntry *)((u64)extCap + extCap->nxtOff * 4);
}

static USB_XHCI_GenerTRB *_getEventTRB(USB_XHCIController *ctrl, int intrId) {
	return DMAS_phys2Virt(ctrl->rtRegs->intrRegs[intrId].eveDeqPtr & (~0x3ful));
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
		printk(WHITE, BLACK, "XHCI: %#018lx:no legacy support.\n");
		return 1;
	}
	u16 prevVal = legacy->data1;
	// bit 8 is the os owned bit
	legacy->data1 |= (1 << 8);
	for (int i = 0; i <= 10; i++) {
		if ((legacy->data1 & ((1 << 8) | (1 << 0))) == (1 << 8)) {
			printk(WHITE, BLACK, "XHCI: %#018lx:success to get ownership. (prevVal:%x)\n", ctrl, prevVal);
			return 1;
		} 
		Intr_SoftIrq_Timer_mdelay(2);
	}
	printk(GREEN, BLACK, "XHCI: %#018lx:failed to get ownership.\n", ctrl);
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
	return 1;
}

static int _initPorts(USB_XHCIController *ctrl) {
	// allocate memory for each port
	ctrl->ports = (USB_XHCI_Port *)kmalloc(sizeof(USB_XHCI_Port) * maxPorts(ctrl), 0);
	if (ctrl->ports == 0) {
		printk(RED, BLACK, "XHCI: allocate memory for ports failed\n");
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
		for (int i = 0; i < protocol->portCnt; i++)
			ctrl->ports[protocol->portOff + i - 1].flags |= 
					(protocol->majorRev == 3 ? HW_USB_XHCI_Port_Flag_USB3 : 0) | HW_USB_XHCI_Port_Flag_Master,
			ctrl->ports[protocol->portOff + i - 1].offset = i;
	}
	// set the flag isPaired
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
		}
	}
	return 1;
}

static int _initMem(USB_XHCIController *ctrl) {
	// allocate the device context base address array
	void *addr = _alloc(ctrl, 2048);
	if (addr == NULL) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }
	ctrl->opRegs->devCtxBaseAddr = DMAS_virt2Phys(addr);
	ctrl->devCtx = addr;
	memset(ctrl->devCtx, 0, sizeof(void *) * 2048);
	printk(WHITE, BLACK, "XHCI: %#018lx:devCtxBaseAddr:%#018lx\n", ctrl, ctrl->opRegs->devCtxBaseAddr);
	// allocate the Device Context Data Structure
	for (int i = 1; i < maxSlots(ctrl); i++) {
		addr = _alloc(ctrl, 
				((ctrl->capRegs->hccparam1 & 0x4) ? 64 * 32 : sizeof(USB_XHCI_DeviceSlotContext) + 31 * sizeof(USB_XHCI_EndpointContext)));
		if (addr == NULL) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }
		ctrl->devCtx[i] = addr;
	}
	// allocate command ring of 64 KB
	{
		USB_XHCI_GenerTRB *cmdRing = _alloc(ctrl, Page_4KSize * 16);
		memset(cmdRing, 0, Page_4KSize * 16);
		ctrl->cmdRing = cmdRing;
		ctrl->opRegs->cmdRingCtrl = DMAS_virt2Phys(cmdRing) | 1;
		printk(WHITE, BLACK, "XHCI: %#018lx:cmdRingCtrl:%#018lx\n", ctrl, ctrl->opRegs->cmdRingCtrl);
	}
	// allocate event ring for each interrupter
	ctrl->eveRingSegTbls = _alloc(ctrl, sizeof(USB_XHCI_EveRingSegTblEntry *) * maxIntrs(ctrl));
	for (int i = 0; i < maxIntrs(ctrl); i++) {
		// stop the interrupter
		ctrl->rtRegs->intrRegs[i].mgrRegs = 0x1;
		USB_XHCI_IntrRegs *regs = ctrl->rtRegs->intrRegs + i;
		// allocate 4 segments for one interrupter
		USB_XHCI_EveRingSegTblEntry *segTbl = _alloc(ctrl, sizeof(USB_XHCI_EveRingSegTblEntry) * HW_USB_XHCI_EveRingSegTblSize);
		ctrl->eveRingSegTbls[i] = segTbl;
		for (int tblId = 0; tblId < HW_USB_XHCI_EveRingSegTblSize; tblId++) {
			USB_XHCI_GenerTRB *eveRing = _alloc(ctrl, Page_4KSize * 16);
			ctrl->eveRingSegTbls[i][tblId].addr = (u64)DMAS_virt2Phys(eveRing);
			ctrl->eveRingSegTbls[i][tblId].size = Page_4KSize * 16 / sizeof(USB_XHCI_GenerTRB);
		}
		ctrl->rtRegs->intrRegs[i].mgrRegs = 0x3 | DMAS_virt2Phys(segTbl);
	}
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
			printk(RED, BLACK, "XHCI: reset controller failed\n");
			return 0;
		}
	}
	
	// check the default value
	if (ctrl->opRegs->usbCmd || ctrl->opRegs->devNotifCtrl
			 || ctrl->opRegs->cmdRingCtrl || ctrl->opRegs->devCtxBaseAddr || ctrl->opRegs->config) {
		printk(RED, BLACK, "XHCI: default value check failed\n");
		return 0;
	}
	return 1;
}

int _restartController(USB_XHCIController *ctrl) {
	_setCmdBit(ctrl, 0);
	for (int remain = 30; remain > 0; remain--) {
		Intr_SoftIrq_Timer_mdelay(2);
		if (ctrl->opRegs->usbStatus & 1) continue;
		break;
	}
	if (ctrl->opRegs->usbStatus & 1) {
		printk(RED, BLACK, "XHCI: %#018lx: restart failed\n", ctrl);
		return 0;
	}
	return 1;
}

int _simpleTest(USB_XHCIController *ctrl) {
	if (ctrl->opRegs->usbStatus & ((1 << 11) | (1 << 12) | (1 << 0))) {
		printk(RED, BLACK, "XHCI: %#018lx: Simple test failed\n", ctrl);
		return 0;
	} 
	// write a no operation TRB to the first interrupter and wait for event
	USB_XHCI_GenerTRB *trb = &ctrl->cmdRing[0];
	memset(trb, 0, sizeof(USB_XHCI_GenerTRB));
	trb->dw3.ctx.trbType = 8;
	trb->dw3.ctx.cycle = 1;
	// ring the doorbell
	ctrl->dbRegs->cmd = (ctrl->dbRegs->cmd & ~0xf) | 1;
	for (i32 remain = 100; remain > 0; remain--) {
		Intr_SoftIrq_Timer_mdelay(10);
		if ((ctrl->opRegs->usbStatus | (1 << 3)) && (ctrl->rtRegs->intrRegs[0].mgrRegs & 1))
			break;
	}
	if (!(ctrl->opRegs->usbStatus | (1 << 3)) && (ctrl->rtRegs->intrRegs[0].mgrRegs & 1)) {
		printk(RED, BLACK, "XHCI: %#018lx: Simple test failed\n", ctrl);
		return 0;
	}
	ctrl->opRegs->usbStatus = (1 << 3);
	ctrl->rtRegs->intrRegs[0].mgrRegs = 0x3;
	USB_XHCI_GenerTRB *eveTrb = _getEventTRB(ctrl, 0);
	printk("trbType:%d trb:%#018lx, eveTrb:%#018lx\n", trb, *(u64 *)&eveTrb->dw[0]);
	return 1;
}

int HW_USB_XHCI_Init(PCIeConfig *xhci) {
    USB_XHCIController *ctrl = (USB_XHCIController *)kmalloc(sizeof(USB_XHCIController), 0);
	List_init(&ctrl->listEle), List_init(&ctrl->memList);

	// set the device struct
	ctrl->dev.install = NULL;
	ctrl->dev.uninstall = NULL;
	ctrl->dev.free = (void (*)(Device *))_free;

	ctrl->config = xhci;
	u64 addr = (xhci->type.type0.bar[0] | (((u64)xhci->type.type0.bar[1]) << 32)) & ~0xffful;
	printk(YELLOW, BLACK, "XHCI:%#018lx\n", xhci);
	u64 pldEntry = MM_PageTable_getPldEntry(getCR3(), (u64)DMAS_phys2Virt(addr));
	if ((pldEntry & ~0xffful) == 0) MM_PageTable_map2M(getCR3(), Page_2MDownAlign((u64)DMAS_phys2Virt(addr)), Page_2MDownAlign(addr), MM_PageTable_Flag_Presented | MM_PageTable_Flag_Writable);
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
	printk(YELLOW, BLACK, "capReg:%#018lx opRegs:%#018lx rtRegs:%#018lx dbRegs:%#018lx extCap:%#018lx Slot:%d Intr:%d port:%d\n",
			ctrl->capRegs, ctrl->opRegs, ctrl->rtRegs, ctrl->dbRegs, ctrl->extCapHeader,
			maxSlots(ctrl), maxIntrs(ctrl), maxPorts(ctrl));

	int state = _getOwnership(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	state = _stopController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// pair up the USB3 and USB2
	state = _initPorts(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// allocate memory for the controller
	state = _initMem(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	// reset the controller
	state = _resetController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	state = _restartController(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	state = _simpleTest(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	List_insBefore(&ctrl->listEle, &HW_USB_XHCI_mgrList);
	return 1;
}

void HW_USB_XHCI_thread(USB_XHCIController *ctrl) {

}