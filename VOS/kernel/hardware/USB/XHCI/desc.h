#ifndef __HW_USB_XHCI_DESC_H__
#define __HW_USB_XHCI_DESC_H__

#include "../../../includes/lib.h"

#define HW_USB_XHCI_DescType_Device		0x1
#define HW_USB_XHCI_DescType_Config		0x2
#define HW_USB_XHCI_DescType_String		0x3
#define HW_USB_XHCI_DescType_Interface	0x4
#define HW_USB_XHCI_DescType_Endpoint	0x5
#define HW_USB_XHCI_DescType_HID		0x21
#define HW_USB_XHCI_DescType_Report		0x22

typedef struct USB_XHCI_DescHeader {
	u8 len;
	u8 descType;
} __attribute__((packed)) USB_XHCI_DescHeader;
typedef struct USB_XHCI_DevDesc {
	USB_XHCI_DescHeader header;
	
	u16 bcdUSB;
	u8 class;
	u8 subClass;
	u8 proto;
	u8 mxPkt0;
	u16 vendorId;
	u16 productId;
	u16 bcdDev;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 numConfig;
} __attribute__((packed)) USB_XHCI_DevDesc;

typedef struct USB_XHCI_ConfigDesc {
	USB_XHCI_DescHeader header;

	u16 totLen;
	u8 numInterface;
	u8 configVal;
	u8 iConfig;
	u8 attributes;
	u8 mxPwr;
} __attribute__((packed)) USB_XHCI_ConfigDesc;

typedef struct USB_XHCI_InterfaceDesc {
	USB_XHCI_DescHeader header;

	u8 interfaceNum;
	u8 alterSetting;
	u8 numEndpoint;
	u8 interfaceClass;
	u8 interfaceSubClass;
	u8 interfaceProtocol;
	u8 interface;
} __attribute__((packed)) USB_XHCI_InterfaceDesc;

typedef struct USB_XHCI_EndpointDesc {
	USB_XHCI_DescHeader header;

	u8 epAddr;
	u8 attr;
	u16 mxPktSz;
	u8 interval;
} __attribute__((packed)) USB_XHCI_EndpointDesc;
#endif