#include "inner.h"
#include "ringop.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"

static void _setPortStsCtrl(u32 *addr, u32 val) {
	*addr = (*addr & Port_StatusCtrl_Reserved) | val;
}

static void _portChgEvent(USB_XHCIController *ctrl, u32 port) {
	if (!(ctrl->ports[port].regs->statusCtrl & 1)) {
		printk(WHITE, BLACK, "XHCI: %#018lx: Port %d disconnected.\n", ctrl, port);
		return ;
	}
	u8 spdId = (ctrl->ports[port].regs->statusCtrl >> 10) & ((1 << 4) - 1);
	printk(WHITE, BLACK, "XHCI: %#018lx: Port %d connect, speed %d\n", ctrl, port, spdId);
	if (spdId == 0) { printk(WHITE, BLACK, "What?\n"); return 0; }
	
}

u64 HW_USB_XHCI_thread(u64 (*_)(u64), u64 ctrlAddr) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	USB_XHCIController *ctrl = (USB_XHCIController *)ctrlAddr;
	int firPeriod = 1;
	printk(WHITE, BLACK, "HW_USB_XHCI_thread(): %#018lx\n", ctrl);
	while (1) {
		if (!(ctrl->opRegs->usbStatus & (UsbState_EveIntr | UsbState_PortChange)) && List_isEmpty(&ctrl->witReqList) && !firPeriod) {
			IO_hlt();
			continue;
		}
		printk(YELLOW, BLACK, "XHCI: %#018lx: usbState: %#010lx\n", ctrl, ctrl->opRegs->usbStatus);
		if ((ctrl->opRegs->usbStatus & UsbState_PortChange) || firPeriod) {
			ctrl->opRegs->usbStatus = UsbState_PortChange;
			for (int i = 0; i < maxPorts(ctrl); i++) {
				USB_XHCI_Port *port = &ctrl->ports[i];
				if (port->regs->statusCtrl & Port_StatusCtrl_ConnectChange) {
					_setPortStsCtrl(&port->regs->statusCtrl, Port_StatusCtrl_ConnectChange | Port_StatusCtrl_Power | Port_StatusCtrl_GenerAllEve);
					_portChgEvent(ctrl, i);
				}
			}
		}
		if ((ctrl->opRegs->usbStatus & UsbState_EveIntr) || firPeriod) {
			ctrl->opRegs->usbStatus = UsbState_EveIntr;
			for (int i = 0; i < maxIntrs(ctrl); i++) {
				USB_XHCI_GenerTRB *intrTRB;
				if (!(ctrl->rtRegs->intrRegs[i].eveDeqPtr & 0x8)) continue;
				while ((intrTRB = HW_USB_XHCI_getNextEveTRB(ctrl, i)) != NULL)
					printk(YELLOW, BLACK, "XHCI: %#018lx: new Event TRB: type:%d datas:%#018lx\n", ctrl, intrTRB->dw3.ctx.trbType, *(u64 *)intrTRB->dw);
			}
		}
		firPeriod = 0;
	}
}