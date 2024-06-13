#include "XHCI.h"
#include "../../includes/interrupt.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"

#define maxSlots(ctrl) (ctrl->capRegs->hcsParams1 & 0xf)
#define maxIntrs(ctrl) (ctrl->capRegs->hcsParams1 >> 8 & 0x7ff)
#define maxPorts(ctrl) (ctrl->capRegs->hcsParams1 >> 24 & 0xff)
#define ist(ctrl) (ctrl->capRegs->hcsParams2 & 0x7)
#define maxEveRingSegTbl(ctrl) ((ctrl->capRegs->hcsParams2 & 0xf) >> 4);
static inline u64 maxScratchBufs(USB_XHCIController *ctrl) {
	u64 higt = ctrl->capRegs->hcsParams2 >> 21 & 0x1f,
		low = ctrl->capRegs->hcsParams2 >> 27 & 0x1f;
	return (higt << 5) | low;
}
#define ac64(ctrl) (ctrl->capRegs->hcsParams3 & 0x1)

static inline Page *_allocPage(USB_XHCIController *ctrl, u64 log2Size) {
	Page *page = ac64(ctrl) ? MM_Buddy_alloc4G(log2Size, Page_Flag_Kernel) : MM_Buddy_alloc(log2Size, Page_Flag_Kernel);
	// add this page to page list
	List_insBefore(&page->listEle, &ctrl->pageList);
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

void HW_USB_XHCI_Init(PCIeConfig *xhci) {
    USB_XHCIController *ctrl = (USB_XHCIController *)kmalloc(sizeof(USB_XHCIController), 0);
	List_init(&ctrl->listEle), List_init(&ctrl->pageList);
	List_insBefore(&ctrl->listEle, &HW_USB_XHCI_mgrList);

	// set the device struct
	ctrl->dev.install = NULL;
	ctrl->dev.uninstall = NULL;
	ctrl->dev.free = (void (*)(Device *))_free;

	ctrl->config = xhci;
	// get the capability registers
	ctrl->capRegs = (USB_XHCI_CapRegs *)DMAS_phys2Virt(*(u64 *)&xhci->type.type0.bar[0] & ~0xfff);
	// get the operational registers
	ctrl->opRegs = (USB_XHCI_OpRegs *)((u64)ctrl->capRegs + ctrl->capRegs->capLen);
	// get the runtime registers
	ctrl->rtRegs = (USB_XHCI_RuntimeRegs *)((u64)ctrl->capRegs + ctrl->capRegs->rtsoff);
	// get the doorbell registers
	ctrl->dbRegs = (USB_XHCI_DoorbellRegs *)((u64)ctrl->capRegs + ctrl->capRegs->dboff);
	printk(YELLOW, BLACK, "XHCI: %#018lx,opRegs:%#018lx rtRegs:%#018lx dbRegs:%#018lx\n", ctrl->capRegs, ctrl->opRegs, ctrl->rtRegs, ctrl->dbRegs);
	printk(WHITE, BLACK, "slot:%d intr:%d port:%d maxScratchBuf:%ld pageSize:%#018lx\n", 
		maxSlots(ctrl), maxIntrs(ctrl), maxPorts(ctrl), maxScratchBufs(ctrl),
		(ctrl->opRegs->pageSize & 0xffff) << 12);
	// allocate the page for scratch buffer
	if (maxScratchBufs(ctrl) > 0) {
		u64 log2Size = 0, tmp = maxScratchBufs(ctrl);
		while (tmp >>= 1) log2Size++;
		_allocPage(ctrl, log2Size - 12);
	}

	// reset the controller
	// set the run/stop bit to 0 -> stop the controller
	_clrCmdBit(ctrl, 0);
	 // wait 30ms for the controller to stop
	 {
		u64 remain = 30;
		do {
			// wait 5ms
			Intr_SoftIrq_Timer_mdelay(5);
			// check the bit HCHalted of controller status
			if (_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_HCHalted)) break;
		} while (remain -= 5);
		if (!_rdStsBit(ctrl, HW_USB_XHCI_OpReg_Status_HCHalted)) {
			printk(RED, BLACK, "XHCI: stop controller failed\n");
			while (1) IO_hlt();
		}
	 }
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
			while (1) IO_hlt();
		}
	}
	
	// check the default value
	if (ctrl->opRegs->usbCmd || ctrl->opRegs->devNotifCtrl || ctrl->opRegs->cmdRingCtrl || ctrl->opRegs->devCtxBaseAddr || ctrl->opRegs->config) {
		printk(RED, BLACK, "XHCI: default value check failed\n");
		while (1) IO_hlt();
	}
}