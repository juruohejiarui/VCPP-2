#ifndef __HW_USB_XHCI_API_H__
#define __HW_USB_XHCI_API_H__

#include "desc.h"

u64 HW_USB_XHCI_readQuad(u64 addr);
/// @brief read a dword from the xhci host
/// @param addr the virtual address
/// @return the data
u32 HW_USB_XHCI_readDword(u64 addr);
u16 HW_USB_XHCI_readWord(u64 addr);
u8 HW_USB_XHCI_readByte(u64 addr);

void HW_USB_XHCI_writeQuad(u64 addr, u64 val);
void HW_USB_XHCI_writeDword(u64 addr, u32 val);
void HW_USB_XHCI_writeWord(u64 addr, u16 val);
void HW_USB_XHCI_writeByte(u64 addr, u8 val);

static __always_inline__ u32 HW_USB_XHCI_TRB_getType(XHCI_GenerTRB *trb) {
	return (HW_USB_XHCI_readDword((u64)&trb->ctrl) >> 10) & ((1 << 6) - 1);
}
static __always_inline__ void HW_USB_XHCI_TRB_setType(XHCI_GenerTRB *trb, u32 type) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & (0xffff04ffu)) | (type << 10));
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getCmplCode(XHCI_GenerTRB *trb) {
	return (HW_USB_XHCI_readDword((u64)&trb->status) >> 24) & 0xff;
}
static __always_inline__ void HW_USB_XHCI_TRB_setCycBit(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & ~0x1u) | val);
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getCycBit(XHCI_GenerTRB *trb) {
	return HW_USB_XHCI_readDword((u64)&trb->ctrl) & 1;
}
static __always_inline__ void HW_USB_XHCI_TRB_setToggle(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & ~0x2u) | (val << 1));
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getToggle(XHCI_GenerTRB *trb) {
	return (HW_USB_XHCI_readDword((u64)&trb->ctrl) >> 1) & 1;
}
static __always_inline__ u64 HW_USB_XHCI_TRB_getData(XHCI_GenerTRB *trb) {
	return HW_USB_XHCI_readQuad((u64)&trb->data1);
}
static __always_inline__ void HW_USB_XHCI_TRB_setData(XHCI_GenerTRB *trb, u64 data) {
	HW_USB_XHCI_writeQuad((u64)&trb->data1, data);
}
static __always_inline__ int HW_USB_XHCI_TRB_getPos(XHCI_GenerTRB *trb) {
	return ((u64)trb - ((u64)trb & ~0xfff)) / sizeof(XHCI_GenerTRB);
}
void HW_USB_XHCI_TRB_copy(XHCI_GenerTRB *src, XHCI_GenerTRB *dst);

static __always_inline__ u8 HW_USB_XHCI_CapReg_capLen(XHCI_Host *host) {
	return HW_USB_XHCI_readByte(host->capRegAddr + XHCI_CapReg_capLen);
}

static __always_inline__ u16 HW_USB_XHCI_CapReg_hciVer(XHCI_Host *host) {
	return HW_USB_XHCI_readWord(host->capRegAddr + XHCI_CapReg_hciVer);
}

static __always_inline__ u32 HW_USB_XHCI_CapReg_rtsOff(XHCI_Host *host) {
	return HW_USB_XHCI_readDword(host->capRegAddr + XHCI_CapReg_rtsOff) & ~0x1fu;
}

static __always_inline__ u32 HW_USB_XHCI_CapReg_dbOffset(XHCI_Host *host) {
	return HW_USB_XHCI_readDword(host->capRegAddr + XHCI_CapReg_dbOff) & ~0x03u;
}


static __always_inline__ void HW_USB_XHCI_writeOpReg(XHCI_Host *host, u32 offset, u32 val) {
	HW_USB_XHCI_writeDword(host->opRegAddr + offset, val);
}

static __always_inline__ u32 HW_USB_XHCI_readOpReg(XHCI_Host *host, u32 offset) {
	return HW_USB_XHCI_readDword(host->opRegAddr + offset);
}

// set the device context base address array pointer of the operation register set
static __always_inline__ void HW_USB_XHCI_writeDCBAAP(XHCI_Host *host, u64 addr) {
	HW_USB_XHCI_writeQuad(host->opRegAddr + XHCI_OpReg_dcbaa, addr);
}
static __always_inline__ u64 HW_USB_XHCI_readDCBAAP(XHCI_Host *host) {
	return HW_USB_XHCI_readQuad(host->opRegAddr + XHCI_OpReg_dcbaa);
}
static __always_inline__ void HW_USB_XHCI_writePortReg(XHCI_Host *host, int portId, u32 offset, u32 val) {
	HW_USB_XHCI_writeDword(host->opRegAddr + 0x400 + (portId - 1) * 0x10 + offset, val);
}
static __always_inline__ u32 HW_USB_XHCI_readPortReg(XHCI_Host *host, int portId, u32 offset) {
	return HW_USB_XHCI_readDword(host->opRegAddr + 0x400 + (portId - 1) * 0x10 + offset);
}
static __always_inline__ u32 HW_USB_XHCI_readIntrDword(XHCI_Host *host, u32 intrId, u32 offset) {
	return HW_USB_XHCI_readDword(host->rtRegAddr + 0x20 + (intrId) * 0x20 + offset);
}
static __always_inline__ void HW_USB_XHCI_writeIntrDword(XHCI_Host *host, u32 intrId, u32 offset, u32 val) {
	HW_USB_XHCI_writeDword(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}
static __always_inline__ u64 HW_USB_XHCI_readIntrQuad(XHCI_Host *host, u32 intrId, u32 offset) {
	return HW_USB_XHCI_readQuad(host->rtRegAddr + 0x20 + (intrId) * 0x20 + offset);
}
static __always_inline__ void HW_USB_XHCI_writeIntrQuad(XHCI_Host *host, u32 intrId, u32 offset, u64 val) {
	HW_USB_XHCI_writeQuad(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}

#define HW_USB_XHCI_CapReg_hcsParam(host, id) \
	HW_USB_XHCI_readDword((host)->capRegAddr + XHCI_CapReg_hcsParam##id)
#define HW_USB_XHCI_CapReg_hccParam(host, id) \
	HW_USB_XHCI_readDword((host)->capRegAddr + XHCI_CapReg_hccParam##id)

#define HW_USB_XHCI_maxSlot(host) \
	(HW_USB_XHCI_CapReg_hcsParam(host, 1) & ((1 << 8) - 1))
#define HW_USB_XHCI_maxIntr(host) \
	((HW_USB_XHCI_CapReg_hcsParam(host, 1) >> 8) & ((1 << 11) - 1))
#define HW_USB_XHCI_maxPort(host) \
	((HW_USB_XHCI_CapReg_hcsParam(host, 1) >> 24) & ((1 << 8) - 1))
// get max scratchpad buffer size 
u32 HW_USB_XHCI_maxScrSz(XHCI_Host *host);

void *HW_USB_XHCI_getNxtECP(XHCI_Host *host, void *cur);
static __always_inline__ u8 HW_USB_XHCI_ECP_id(void *cur) { return HW_USB_XHCI_readByte((u64)cur); }

// reset the host, return 0 if failed; 1 if successful
int HW_USB_XHCI_reset(XHCI_Host *host);

void HW_USB_XHCI_waiForHostIsReady(XHCI_Host *host);

void HW_USB_XHCI_init(PCIeManager *pci);

// allocate a ring (transfer ring/command ring) with SIZE trbs
XHCI_Ring *HW_USB_XHCI_allocRing(u64 size);

void HW_USB_XHCI_freeRing(XHCI_Ring *ring);

// allocate a event ring array with NUM * SIZE TRBS
XHCI_EveRing *HW_USB_XHCI_allocEveRing(u32 num, u32 size);

void HW_USB_XHCI_freeEveRing(XHCI_EveRing *ring);

XHCI_Request *HW_USB_XHCI_allocReq(u64 trbCnt);

void HW_USB_XHCI_freeReq(XHCI_Request *req);

/// @brief try to insert the request into the specific ring
/// @return 1: success 0: failed because the ring is full
int HW_USB_XHCI_Ring_tryInsReq(XHCI_Ring *ring, XHCI_Request *req);

IntrHandlerDeclare(HW_USB_XHCI_msiHandler);

#endif