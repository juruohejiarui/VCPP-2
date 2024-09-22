#ifndef __HW_USB_XHCI_API_H__
#define __HW_USB_XHCI_API_H__

#include "desc.h"

u64 HW_USB_XHCI_readQuad(u64 addr);
/// @brief read a dword from the xhci host
/// @param addr the virtual address
/// @return the data
static __always_inline__ u32 HW_USB_XHCI_readDword(u64 addr) { 
	u32 val;
	__asm__ volatile (
		"movl (%1), %0	\n\t"
		"mfence			\n\t"
		: "=b"(val)
		: "a"(addr)
		: "memory"
	);
	return val;
 }
u16 HW_USB_XHCI_readWord(u64 addr);
u8 HW_USB_XHCI_readByte(u64 addr);

void HW_USB_XHCI_writeQuad(u64 addr, u64 val);
static __always_inline__ void HW_USB_XHCI_writeDword(u64 addr, u32 val) {
	__asm__ volatile (
		"movl %0, (%1)		\n\t"
		"mfence				\n\t"
		:
		: "a"(val), "b"(addr)
		: "memory"
	);
}
void HW_USB_XHCI_writeWord(u64 addr, u16 val);
void HW_USB_XHCI_writeByte(u64 addr, u8 val);

static __always_inline__ u32 HW_USB_XHCI_EpDesc_epId(XHCI_EpDesc *epDesc) {
	return ((epDesc->bEpAddr & 0xf) << 1) + (epDesc->bEpAddr >> 7) - 1;
}


static __always_inline__ u32 HW_USB_XHCI_TRB_getType(XHCI_GenerTRB *trb) {
	return (HW_USB_XHCI_readDword((u64)&trb->ctrl) >> 10) & ((1 << 6) - 1);
}
static __always_inline__ void HW_USB_XHCI_TRB_setType(XHCI_GenerTRB *trb, u32 type) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & (0xffff03ffu)) | (type << 10));
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
static __always_inline__ void HW_USB_XHCI_TRB_setCtrlBit(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, val | (HW_USB_XHCI_readDword((u64)&trb->ctrl) & ~XHCI_TRB_Ctrl_allBit));
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getCtrlBit(XHCI_GenerTRB *trb) {
	return HW_USB_XHCI_readDword((u64)&trb->ctrl) & XHCI_TRB_Ctrl_allBit;
}
static __always_inline__ u32 HW_USB_XHCI_TRB_setTRT(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (val << 16) | (HW_USB_XHCI_readDword((u64)&trb->ctrl) & 0xfffcffffu));
}
static __always_inline__ void HW_USB_XHCI_TRB_setDir(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_TRB_setTRT(trb, val & 0x1);
}
static __always_inline__ u64 HW_USB_XHCI_TRB_getData(XHCI_GenerTRB *trb) {
	return HW_USB_XHCI_readQuad((u64)&trb->data1);
}
static __always_inline__ void HW_USB_XHCI_TRB_setData(XHCI_GenerTRB *trb, u64 data) {
	HW_USB_XHCI_writeQuad((u64)&trb->data1, data);
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getIntrTarget(XHCI_GenerTRB *trb) {
	// interrupt target and completion code are in the same field
	return HW_USB_XHCI_TRB_getCmplCode(trb);
}
static __always_inline__ void HW_USB_XHCI_TRB_setIntrTarget(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->status, (HW_USB_XHCI_readDword((u64)&trb->status) & 0xffffffu) | (val << 24));
}
static __always_inline__ u64 HW_USB_XHCI_TRB_setStatus(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->status, val);
}
static __always_inline__ u64 HW_USB_XHCI_TRB_mkSetup(u8 bmReqT, u8 bReq, u16 wVal, u16 wIdx, u16 wLen) {
	return (u64)bmReqT | (((u64)bReq) << 8) | (((u64)wVal) << 16) | (((u64)wIdx) << 32) | (((u64)wLen) << 48);
}
static __always_inline__ u32 HW_USB_XHCI_TRB_mkStatus(u32 trbTransLen, u32 tdSize, u32 intrTarget) {
	return trbTransLen | (tdSize << 17) | (intrTarget << 22);
}
static __always_inline__ u32 HW_USB_XHCI_TRB_getSlot(XHCI_GenerTRB *trb) {
	return HW_USB_XHCI_readDword((u64)&trb->ctrl) >> 24;
}
static __always_inline__ u32 HW_USB_XHCI_TRB_setSlot(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & 0xffffffu) | (val << 24));
}
static __always_inline__ void HW_USB_XHCI_TRB_setBSR(XHCI_GenerTRB *trb, u32 val) {
	HW_USB_XHCI_writeDword((u64)&trb->ctrl, (HW_USB_XHCI_readDword((u64)&trb->ctrl) & ~(1u << 9)) | (val << 9));
}
static __always_inline__ int HW_USB_XHCI_TRB_getPos(XHCI_GenerTRB *trb) {
	return ((u64)trb - ((u64)trb & ~0xfffful)) / sizeof(XHCI_GenerTRB);
}
void HW_USB_XHCI_TRB_copy(XHCI_GenerTRB *src, XHCI_GenerTRB *dst);

XHCI_DescHdr *HW_USB_XHCI_Desc_nxtCfgItem(XHCI_CfgDesc *cfg, XHCI_DescHdr *cur);

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
static __always_inline__ void HW_USB_XHCI_writeOpRegQuad(XHCI_Host *host, u32 offset, u64 val) {
	HW_USB_XHCI_writeQuad(host->opRegAddr + offset, val);
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

void HW_USB_XHCI_portConnect(XHCI_Host *host, int portId);
void HW_USB_XHCI_portDisconnect(XHCI_Host *host, int portId);

static __always_inline__ u32 HW_USB_XHCI_readIntrDword(XHCI_Host *host, u32 intrId, u32 offset) {
	return HW_USB_XHCI_readDword(host->rtRegAddr + 0x20 + (intrId) * 0x20 + offset);
}
static __always_inline__ void HW_USB_XHCI_writeIntrDword(XHCI_Host *host, u32 intrId, u32 offset, u32 val) {
	HW_USB_XHCI_writeDword(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}
static __always_inline__ u64 HW_USB_XHCI_readIntrQuad(XHCI_Host *host, u32 intrId, u32 offset) {
	return HW_USB_XHCI_readQuad(host->rtRegAddr + 0x20 + intrId * 0x20 + offset);
}
static __always_inline__ void HW_USB_XHCI_writeIntrQuad(XHCI_Host *host, u32 intrId, u32 offset, u64 val) {
	HW_USB_XHCI_writeQuad(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}

static __always_inline__ void HW_USB_XHCI_writeDbReg(XHCI_Host *host, u32 slotId, u32 epId, u32 taskId) {
	__asm__ volatile ( "mfence \n\t" : : : "memory");
	HW_USB_XHCI_writeDword(host->dbRegAddr + slotId * 0x4, epId | (taskId << 16));
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

// allocate a ring (transfer ring/command ring) with SIZE trbs with parameter Slab_Flag_Private
XHCI_Ring *HW_USB_XHCI_allocRing(u64 size);

void HW_USB_XHCI_freeRing(XHCI_Ring *ring);

/// @brief try to insert the request into the specific ring
/// @return 1: success 0: failed because the ring is full
int HW_USB_XHCI_Ring_tryInsReq(XHCI_Ring *ring, XHCI_Request *req);

// try to insert the request into the ring until successful.
void HW_USB_XHCI_Ring_insReq(XHCI_Ring *ring, XHCI_Request *req);

void HW_USB_XHCI_Ring_reset(XHCI_Ring *ring);

// release the occurpancy from the request on POS and return the pointer of that request
XHCI_Request *HW_USB_XHCI_Ring_release(XHCI_Ring *ring, int pos);

// allocate a event ring array with NUM * SIZE TRBS with parameter Slab_Flag_Private
XHCI_EveRing *HW_USB_XHCI_allocEveRing(u32 num, u32 size);

void HW_USB_XHCI_freeEveRing(XHCI_EveRing *ring);

int HW_USB_XHCI_EveRing_getNxt(XHCI_EveRing *ring, XHCI_GenerTRB **trb);

// allocate a request block with TRB_CNT trbs with parameter Slab_Flag_Private
XHCI_Request *HW_USB_XHCI_allocReq(u64 trbCnt);

void HW_USB_XHCI_freeReq(XHCI_Request *req);

// initialize the request without data stage
void HW_USB_XHCI_ctrlReq(XHCI_Request *req, u64 setup, int dir);

// initialize the request with data stage
void HW_USB_XHCI_ctrlDataReq(XHCI_Request *req, u64 setup, int dir, void *data, u16 len);

// wait for the result of request and return the completion code
int HW_USB_XHCI_Req_wait(XHCI_Request *req);

// ring the doorbell of HOST, wait for interrupt result and finally return the completion code
int HW_USB_XHCI_Req_ringDbWait(XHCI_Host *host, u32 slotId, u32 epId, u32 taskId, XHCI_Request *req);

void HW_USB_XHCI_writeCtx(void *ctx, int dwId, u32 mask, u32 val);

u32 HW_USB_XHCI_readCtx(void *ctx, int dwId, u32 mask);

u32 HW_USB_XHCI_EpCtx_readMxESITPay(XHCI_EpCtx *ep);

void HW_USB_XHCI_EpCtx_writeMxESITPay(XHCI_EpCtx *ep, u32 val);

// get the max packet size for endpoint 0 (control endpoint)
u32 HW_USB_XHCI_EpCtx_getMxPackSize0(u32 speed);

IntrHandlerDeclare(HW_USB_XHCI_msiHandler);

void HW_USB_XHCI_init(PCIeManager *pci);

void HW_USB_XHCI_evehandleTask(XHCI_Host *host, u64 intrId);

void HW_USB_XHCI_devMgrTask(XHCI_Device *dev, u64 rootPort);

void HW_USB_XHCI_test(XHCI_Host *host);

#endif