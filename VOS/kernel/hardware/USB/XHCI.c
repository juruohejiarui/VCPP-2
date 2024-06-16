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

static inline Page *_allocPage(USB_XHCIController *ctrl, u64 log2Size) {
	Page *page = ac64(ctrl) ? MM_Buddy_alloc4G(log2Size, Page_Flag_Kernel) : MM_Buddy_alloc(log2Size, Page_Flag_Kernel);
	if (page == NULL) {
		printk(RED, BLACK, "XHCI: allocate page failed log2Size:%ld\n", log2Size);
		return NULL;
	}
	// add this page to page list
	List_insBefore(&page->listEle, &ctrl->pageList);
}

// allocate memory for the controller，use DMAS_virt2Phys to get the physical address
static void *_alloc(USB_XHCIController *ctrl, u64 size) {
	if (size > Page_4KSize) return NULL;
	void *addr = NULL;
	if (size > MM_Slab_maxSize) {
		Page *page = _allocPage(ctrl, 0);
		if (page == NULL) return NULL;
		addr = DMAS_phys2Virt(page->phyAddr);
	} else addr = kmalloc(size, 0);
	return addr;
}

List HW_USB_XHCI_mgrList;

static void _free(USB_XHCIController *ctrl) {
	// free all pages
	Page *page;
	while (!List_isEmpty(&ctrl->pageList)) {
		page = container(ctrl->pageList.next, Page, listEle);
		List_del(&page->listEle);
		MM_Buddy_free(page);
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

extern USB_XHCI_ExtCapEntry *_getNextExtCap(USB_XHCI_ExtCapEntry *extCap) {
	if (extCap->nxtOff == 0) return NULL;
	return (USB_XHCI_ExtCapEntry *)((u64)extCap + extCap->nxtOff * 4);
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
			ctrl->ports[protocol->portOff + i - 1].flags |= (protocol->majorRev == 3 ? HW_USB_XHCI_Port_Flag_USB3 : 0) | HW_USB_XHCI_Port_Flag_Master,
			ctrl->ports[protocol->portOff + i - 1].offset = i;
	}
	// set the flag isPaired
	for (int i = 0; i < maxPorts(ctrl); i++) {
		for (int j = i + 1; j < maxPorts(ctrl); j++) {
			if (ctrl->ports[i].offset != ctrl->ports[j].offset) continue;
			ctrl->ports[i].flags |= HW_USB_XHCI_Port_Flag_Paired;
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
	printk(WHITE, BLACK, "XHCI: devCtxBaseAddr:%#018lx\n", ctrl->opRegs->devCtxBaseAddr);
	// allocate the Device Context Data Structure
	for (int i = 1; i < maxSlots(ctrl); i++) {
		addr = _alloc(ctrl, ((ctrl->capRegs->hccparam1 & 0x4) ? 64 * 32 : sizeof(USB_XHCI_DeviceSlotContext) + 31 * sizeof(USB_XHCI_EndpointContext)));
		if (addr == NULL) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }
		ctrl->devCtx[i] = addr;
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
	if (ctrl->opRegs->usbCmd || ctrl->opRegs->devNotifCtrl || ctrl->opRegs->cmdRingCtrl || ctrl->opRegs->devCtxBaseAddr || ctrl->opRegs->config) {
		printk(RED, BLACK, "XHCI: default value check failed\n");
		return 0;
	}
	return 1;
}

static int _resetPorts(USB_XHCIController *ctrl) {
	return 1;
}

int HW_USB_XHCI_Init(PCIeConfig *xhci) {
    USB_XHCIController *ctrl = (USB_XHCIController *)kmalloc(sizeof(USB_XHCIController), 0);
	List_init(&ctrl->listEle), List_init(&ctrl->pageList);
	List_insBefore(&ctrl->listEle, &HW_USB_XHCI_mgrList);

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

	int state = _stopController(ctrl);
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

	// setup the context data structure for the controller
	state = _resetPorts(ctrl);
	if (!state) { ctrl->dev.free((Device *)ctrl); kfree(ctrl); return 0; }

	return 1;
}