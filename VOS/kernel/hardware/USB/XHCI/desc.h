#ifndef __HW_USB_XHCI_DESC_H__
#define __HW_USB_XHCI_DESC_H__

#include "../../../includes/lib.h"

typedef struct USB_XHCI_DevDesc {
	u8 len;
	u8 descType;
	
	u16 bcdUSB;
	u8 class;
	u8 subClass;
	u8 proto;
	u8 mkPkt0;
	u16 vendorId;
	u16 productId;
	u16 bcdDev;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 numConfig;
} __attribute__((packed)) USB_XHCI_DevDesc;

typedef struct USB_XHCI_ConfigDesc {
	u8 len;
	u8 descType;

	u16 totLen;
	u8 numInterface;
	u8 iConfig;
	u8 attributes;
	u8 mxPwr;
} __attribute__((packed)) USB_XHCI_ConfigDesc;

typedef struct USB_XHCI_InterfaceDesc {
	u8 len;
	u8 descType;

	u8 interfaceNum;
	u8 alterSetting;
	u8 numEndpoint;
	u8 interfaceClass;
	u8 interfaceSubClass;
	u8 interfaceProtocol;
	u8 interface;
} __attribute__((packed)) USB_XHCI_InterfaceDesc;
#endif