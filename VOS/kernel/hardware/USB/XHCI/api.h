#ifndef __HW_USB_XHCI_API_H__
#define __HW_USB_XHCI_API_H__

#include "../XHCI.h"

void HW_USB_XHCI_insReqBlk(USB_XHCIController *ctrl, USB_XHCIReqBlock *reqs);

int HW_USB_XHCI_waitRely(USB_XHCIController *ctrl, USB_XHCIReqBlock *reqs);

USB_XHCIReqBlock *HW_USB_XHCI_mkCmdBlk(int trbType, u64 slot, u64 arg);

#define HW_USB_XHCI_SetupPkt_DescType_Device	0x1
#define HW_USB_XHCI_SetupPkt_DescType_Config	0x2
#define HW_USB_XHCI_SetupPkt_DescType_String	0x3

USB_XHCIReqBlock *HW_USB_XHCI_mkGetDescBlk(u64 slot, u64 descType, u64 idx, u64 wIdx, u64 len, void *buf);

USB_XHCIReqBlock *HW_USB_XHCI_mkGetDataBlk(u64 slot, u64 epId, u64 len, void *buf);

USB_XHCIReqBlock *HW_USB_XHCI_mkSetDataBlk(u64 slot, u64 epId, u64 len, void *buf);

void HW_USB_XHCI_freeReqBlk(USB_XHCIReqBlock *reqs);
#endif