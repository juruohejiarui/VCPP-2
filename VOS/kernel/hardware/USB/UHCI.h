#ifndef __HAREWARE_USB_UHCI_H__
#define __HAREWARE_USB_UHCI_H__

#include "../PCIe.h"

// I/O registers
typedef struct {
    u16 usbCmd;
    u16 usbStatus;
    u16 usbIntrEnbl;
    u16 frNum;
    u32 frBaseAddr;
    u8 sofMod;
    u8 reserved[3];
    u16 portSts[2];
} __attribute__ ((packed)) USB_UHCI_IORegs;

typedef struct {
    List listEle;
    PCIeConfig *config;
    PCIePowerRegs *pwRegs;
    u32 ioRegAddr;
} USB_UHCIController;

#define HW_USB_UHCI_ioReg_cmd           0x00
#define HW_USB_UHCI_ioReg_status        0x02
#define HW_USB_UHCI_ioReg_intrEnable    0x04
#define HW_USB_UHCI_ioReg_frNum         0x06
#define HW_USB_UHCI_ioReg_frBaseAddr    0x08
#define HW_USB_UHCI_ioReg_frModSt       0x0c
#define HW_USB_UHCI_ioReg_PortSC(x)     (0x10 + (x) * 2)


extern List HW_USB_UHCI_mgrList;

void HW_USB_UHCI_init(PCIeConfig *uhci);

#endif