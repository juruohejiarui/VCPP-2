#include "UHCI.h"
#include "../../includes/interrupt.h"
#include "../../includes/memory.h"
#include "../../includes/log.h"

List HW_USB_UHCI_mgrList;

static void _detect(USB_UHCIController *ctrl) {
	// do reset 5 times
	for (int i = 0; i < 5; i++) {
		IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_cmd, 0x0004);
		Intr_SoftIrq_Timer_mdelay(11);
		IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_cmd, 0x0000);
	}
	if (IO_in16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_cmd) != 0x0000) {
		printk(RED, BLACK, "UHCI ERROR: reset failed\n");
		while (1) IO_hlt();
	}
	if (IO_in16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_status) != 0x0020) {
		printk(RED, BLACK, "UHCI ERROR: invalid status\n");
		while (1) IO_hlt();
	}
	IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_status, 0x00ff);
	if (IO_in16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_frModSt) != 0x40) {
		printk(RED, BLACK, "UHCI ERROR: invalid start of frame modify %#06x\n", IO_in16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_frModSt));
		while (1) IO_hlt();
	}
	// host control reset
	IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_cmd, 0x0002);
	while (IO_in16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_cmd) & 0x0002) Intr_SoftIrq_Timer_mdelay(5);	
}

static void _setup(USB_UHCIController *ctrl) {
	IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_intrEnable, 0x000f);
	IO_out16(ctrl->ioRegAddr + HW_USB_UHCI_ioReg_frNum, 0x0000);
	// allocate frame list
	
}

void HW_USB_UHCI_init(PCIeConfig *uhci) {
	printk(YELLOW, BLACK, "UHCI: %#018lx\t", uhci);
	USB_UHCIController *ctrl = (USB_UHCIController *)kmalloc(sizeof(USB_UHCIController), 0);
	List_init(&ctrl->listEle);
	List_insBefore(&ctrl->listEle, &HW_USB_UHCI_mgrList);
	ctrl->config = uhci;
	printk(WHITE, BLACK, "mapInfo:%02x status:%04x ", (uhci->type.type0.bar[0] & 0xf), (uhci->status));
	// get power registers
	if (uhci->status & 0x10) {
		printk(WHITE, BLACK, "capPtr:%02x\n", uhci->type.type0.capPtr);
		ctrl->pwRegs = (PCIePowerRegs *)(((u64)uhci) + (uhci->type.type0.capPtr & 0xfc));
	} else {
		printk(WHITE, BLACK, "No power registers\n");
		while (1) IO_hlt();
	}
	ctrl->pwRegs->powerCtrl &= 0xfffc;
	Intr_SoftIrq_Timer_mdelay(101);
	printk(WHITE, BLACK, "Power Status:%04x Power Control:%04x\n", ctrl->pwRegs->powerStatus, ctrl->pwRegs->powerCtrl);
	// get I/O registers
	ctrl->ioRegAddr = uhci->type.type0.bar[4] & 0xfffc;
	_detect(ctrl);
	_setup(ctrl);
}