#include "glo.h"
#include "XHCI.h"
#include "EHCI.h"
#include "UHCI.h"
#include "../../includes/log.h"

void HW_USB_init() {
    printk(RED, BLACK, "HW_USB_init()\n");
    List_init(&HW_USB_UHCI_mgrList);
    List *pcieListHeader = HW_PCIe_getMgrList();
    for (List *pcieListEle = pcieListHeader->next; pcieListEle != pcieListHeader; pcieListEle = pcieListEle->next) {
        PCIeManager *mgrStruct = container(pcieListEle, PCIeManager, listEle);
        if (mgrStruct->device->classCode != 0x0c || mgrStruct->device->subclass != 0x03)
            continue;
        switch (mgrStruct->device->progIF) {
            case 0x20:
                HW_USB_EHCI_init(mgrStruct->device);
                break;
            case 0x30:
                HW_USB_XHCI_Init(mgrStruct->device);
                break;
            default:
                HW_USB_UHCI_init(mgrStruct->device);
                break;
        }
    }
}