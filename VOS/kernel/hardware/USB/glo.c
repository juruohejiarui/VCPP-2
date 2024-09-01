#include "glo.h"
#include "XHCI.h"
#include "EHCI.h"
#include "UHCI.h"
#include "HID.h"
#include "../../includes/log.h"

void HW_USB_init() {
    printk(RED, BLACK, "HW_USB_init()\n");
    List_init(&HW_USB_UHCI_mgrList);
	List_init(&HW_USB_XHCI_hostList);
	List_init(&HW_USB_XHCI_DriverList);
	SpinLock_init(&HW_USB_XHCI_DriverListLock);

	HW_USB_HID_init();

    List *pcieListHeader = HW_PCIe_getMgrList();
    for (List *pcieListEle = pcieListHeader->next; pcieListEle != pcieListHeader; pcieListEle = pcieListEle->next) {
        PCIeManager *mgrStruct = container(pcieListEle, PCIeManager, listEle);
        if (mgrStruct->cfg->classCode != 0x0c || mgrStruct->cfg->subclass != 0x03)
            continue;
        switch (mgrStruct->cfg->progIF) {
            case 0x20:
                HW_USB_EHCI_init(mgrStruct->cfg);
                break;
            case 0x30:
                HW_USB_XHCI_init(mgrStruct);
                break;
            default:
                HW_USB_UHCI_init(mgrStruct->cfg);
                break;
        }
    }
}