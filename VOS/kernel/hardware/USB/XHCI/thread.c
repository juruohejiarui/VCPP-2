#include "inner.h"
#include "../../../includes/task.h"
#include "../../../includes/log.h"

u64 HW_USB_XHCI_thread(u64 (*_)(u64), u64 ctrlAddr) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	USB_XHCIController *ctrl = (USB_XHCIController *)ctrlAddr;
	printk(WHITE, BLACK, "HW_USB_XHCI_thread(): %#018lx %#018lx\n", ctrl);
	while (1) {
	}
}