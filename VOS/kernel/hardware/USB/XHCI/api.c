#include "api.h"
#include "../../../includes/log.h"

void HW_USB_XHCI_TRB_copy(XHCI_GenerTRB *src, XHCI_GenerTRB *dst) {
	HW_USB_XHCI_writeQuad((u64)dst, *(u64 *)src);
	HW_USB_XHCI_writeQuad((u64)dst + 0x8, *(u64 *)&src->status);
}

XHCI_DescHdr *HW_USB_XHCI_Desc_nxtCfgItem(XHCI_CfgDesc *cfg, XHCI_DescHdr *cur) {
	if ((u64)cur - (u64)cfg + cur->len >= cfg->wtotLen) return NULL;
	return (XHCI_DescHdr *)((u64)cur + cur->len);
}

u64 HW_USB_XHCI_readQuad(u64 addr) {
	return HW_USB_XHCI_readDword(addr) | (((u64)HW_USB_XHCI_readDword(addr + 0x4)) << 32);
}

u16 HW_USB_XHCI_readWord(u64 addr) {
	u32 data = HW_USB_XHCI_readDword(addr & ~0x3);
	return (data >> ((addr & 0x3) << 3)) & 0xffff;
}

u8 HW_USB_XHCI_readByte(u64 addr) {
	u32 data = HW_USB_XHCI_readDword(addr & ~0x3);
	return (data >> ((addr & 0x3) << 3)) & 0xff;
}

void HW_USB_XHCI_writeQuad(u64 addr, u64 val) {
	HW_USB_XHCI_writeDword(addr, val & ((1ul << 32) - 1));
	HW_USB_XHCI_writeDword(addr + 0x4, (val >> 32) & ((1ul << 32) - 1));
}

void HW_USB_XHCI_writeWord(u64 addr, u16 val) {
	u32 prev = HW_USB_XHCI_readDword(addr & ~0x3);
	u32 mask = 0xffff << ((addr & 0x3) << 3);
	HW_USB_XHCI_writeDword(addr & ~0x3, (prev & ~mask) | ((u32)val << ((addr & 0x3) << 3)));
}

void HW_USB_XHCI_writeByte(u64 addr, u8 val) {
	u32 prev = HW_USB_XHCI_readDword(addr & ~0x3);
	u32 mask = 0xff << ((addr & 0x3) << 3);
	HW_USB_XHCI_writeDword(addr & ~0x3, (prev & ~mask) | ((u32)val << ((addr & 0x3) << 3)));
}

u32 HW_USB_XHCI_maxScrSz(XHCI_Host *host) {
	u32 val = HW_USB_XHCI_CapReg_hcsParam(host, 2);
	const u32 mask = (1u << 5) - 1;
	return (((val >> 21) & mask) << 5) | ((val >> 27) & mask);
}

int HW_USB_XHCI_reset(XHCI_Host *host) {
	int timeout;
	HW_USB_XHCI_writeOpReg(host, XHCI_OpReg_cmd, (1 << 1));
	timeout = 500;
	while ((HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cmd) & (1 << 1)) && timeout > 0) {
		Intr_SoftIrq_Timer_mdelay(1);
		timeout--;
	}
	if ((HW_USB_XHCI_readOpReg(host, XHCI_OpReg_cmd) & (1 << 1)) || 
		!(HW_USB_XHCI_readOpReg(host, XHCI_OpReg_status) & (1 << 0))) return 0;
	return 1;
}

void HW_USB_XHCI_waiForHostIsReady(XHCI_Host *host) {
	while (HW_USB_XHCI_readOpReg(host, XHCI_OpReg_status) & (1 << 11)) Intr_SoftIrq_Timer_mdelay(2);
}


void *HW_USB_XHCI_getNxtECP(XHCI_Host *host, void *cur) {
	if (!cur) return (void *)(host->capRegAddr + (HW_USB_XHCI_CapReg_hccParam(host, 1) >> 16) * 4);
	u32 offset = HW_USB_XHCI_readByte((u64) cur + 1);
	return offset ? (void *)((u64)cur + offset * 4) : NULL;
}

void HW_USB_XHCI_freeRing(XHCI_Ring *ring) {
	if (ring->ring) kfree(ring, 0);
	if (ring->reqSrc) kfree(ring, 0);
}

XHCI_Ring *HW_USB_XHCI_allocRing(u64 size) {
	XHCI_Ring *ring = kmalloc(sizeof(XHCI_Ring), Slab_Flag_Clear | Slab_Flag_Private, (void *)HW_USB_XHCI_freeRing);
	ring->ring = kmalloc(sizeof(XHCI_GenerTRB) * size, Slab_Flag_Clear, NULL);
	ring->reqSrc = kmalloc(sizeof(XHCI_Request *) * size, Slab_Flag_Clear, NULL);
	ring->cur = ring->ring;
	ring->cycBit = 1;
	ring->size = size;

	// make a link TRB at the end and point to the first TRB
	XHCI_GenerTRB *lk = &ring->ring[size - 1];
	HW_USB_XHCI_TRB_setData(lk, DMAS_virt2Phys(&ring->ring[0]));
	HW_USB_XHCI_TRB_setType(lk, XHCI_TRB_Type_Link);
	HW_USB_XHCI_TRB_setToggle(lk, 1);
	SpinLock_init(&ring->lock);
	return ring;
}

int HW_USB_XHCI_Ring_tryInsReq(XHCI_Ring *ring, XHCI_Request *req) {
	// printk(WHITE, BLACK, "waiting for inserting %#018lx into ring %#018lx\n", req, ring);
	// software may use a request structure for multiple times
	req->flags &= ~XHCI_Request_Flag_Finished;
	SpinLock_lock(&ring->lock);
	u64 pos[16]; u8 cyc[16];
	int trbC = 0, full = 0;
	XHCI_GenerTRB *lstCur = ring->cur;
	int lstPos = ring->curPos, lstCyc = ring->cycBit;
	for (int i = 0; i < req->trbCnt; i++) {
		pos[trbC] = ring->curPos, cyc[trbC++] = ring->cycBit;
		if (HW_USB_XHCI_TRB_getType(ring->cur) == XHCI_TRB_Type_Link) {
			if (HW_USB_XHCI_TRB_getToggle(ring->cur)) ring->cycBit ^= 1;
			ring->cur = DMAS_phys2Virt(HW_USB_XHCI_TRB_getData(ring->cur));
			ring->curPos = HW_USB_XHCI_TRB_getPos(ring->cur);
			i--;
			continue;
		}
		// Unhandled request still occupies this trb, means this ring is full
		if (ring->reqSrc[ring->curPos]) {
			full = 1;
			break;
		}
		ring->cur++;
		ring->curPos++;
	}
	if (full) {
		// restore and return fail code
		ring->cur = lstCur, ring->curPos = lstPos, ring->cycBit = lstCyc;
		SpinLock_unlock(&ring->lock);
		printk(RED, BLACK, "ring %#018lx full\n", ring);
		return 0;
	}
	for (int i = 0, reqP = 0; i < trbC; i++) {
		XHCI_GenerTRB *trb = &ring->ring[pos[i]];
		if (HW_USB_XHCI_TRB_getType(trb) != XHCI_TRB_Type_Link) {
			HW_USB_XHCI_TRB_copy(&req->trb[reqP], trb);
			ring->reqSrc[pos[i]] = req;
			req->target[reqP] = &ring->reqSrc[pos[i]];
			reqP++;
		}
		HW_USB_XHCI_TRB_setCycBit(trb, cyc[i]);
	}
	SpinLock_unlock(&ring->lock);
	return 1;
}

// try to insert the request into the ring until successful.
void HW_USB_XHCI_Ring_insReq(XHCI_Ring *ring, XHCI_Request *req) {
	while (!HW_USB_XHCI_Ring_tryInsReq(ring, req)) IO_hlt();
}

void HW_USB_XHCI_Ring_reset(XHCI_Ring *ring) {
	SpinLock_lock(&ring->lock);
	ring->cur = &ring->ring[0];
	ring->curPos = 0, ring->cycBit = 1;
	for (int i = 0; i < ring->size - 1; i++)
		HW_USB_XHCI_writeQuad((u64)&ring->ring[0], 0),
		HW_USB_XHCI_writeQuad((u64)&ring->ring[0] + 0x08, 0);
	memset(ring->reqSrc, 0, sizeof(XHCI_Request *) * ring->size);
	SpinLock_unlock(&ring->lock);
}

XHCI_Request *HW_USB_XHCI_Ring_release(XHCI_Ring *ring, int pos) {
	SpinLock_lock(&ring->lock);
	XHCI_Request *req = ring->reqSrc[pos];
	for (int i = 0; i < req->trbCnt; i++) *req->target[i] = NULL;
	SpinLock_unlock(&ring->lock);
	return req;
}

void HW_USB_XHCI_freeEveRing(XHCI_EveRing *ring) {
	for (int i = 0; i < ring->ringNum; i++)
		kfree(ring->rings[i], 0);
	kfree(ring->rings, 0);
	kfree(ring, 0);
}

XHCI_EveRing *HW_USB_XHCI_allocEveRing(u32 num, u32 size) {
	XHCI_EveRing *ring = kmalloc(sizeof(XHCI_EveRing), Slab_Flag_Clear | Slab_Flag_Private, (void *)HW_USB_XHCI_freeEveRing);
	ring->ringNum = num, ring->ringSize = size;
	ring->cycBit = 1;
	ring->curRingId = ring->curPos = 0;
	ring->rings = kmalloc(sizeof(XHCI_GenerTRB *) * num, Slab_Flag_Clear, NULL);
	for (int i = 0; i < num; i++) ring->rings[i] = kmalloc(size * sizeof(XHCI_GenerTRB), Slab_Flag_Clear, NULL);
	return ring;
}

int HW_USB_XHCI_EveRing_getNxt(XHCI_EveRing *ring, XHCI_GenerTRB **trb) {
	XHCI_GenerTRB *tmp = &ring->rings[ring->curRingId][ring->curPos];
	*trb = tmp;
	if (HW_USB_XHCI_TRB_getCycBit(tmp) != ring->cycBit) return 0;
	if ((++ring->curPos) == ring->ringSize) {
		if ((++ring->curRingId) == ring->ringNum) {
			ring->curRingId = 0;
			ring->cycBit ^= 1;
		}
		ring->curPos = 0;
	}
	return 1;
}


void HW_USB_XHCI_freeReq(XHCI_Request *req) {
	if (req->trb) kfree(req->trb, 0);
	if (req->target) kfree(req->target, 0);
}

void HW_USB_XHCI_ctrlReq(XHCI_Request *req, u64 setup, int dir) {
	{
		XHCI_GenerTRB *setupStage = &req->trb[0];
		HW_USB_XHCI_TRB_setData(setupStage, 	setup);
		HW_USB_XHCI_TRB_setStatus(setupStage, 	HW_USB_XHCI_TRB_mkStatus(0x08, 0, 0));
		HW_USB_XHCI_TRB_setType(setupStage, 	XHCI_TRB_Type_SetupStage);
		HW_USB_XHCI_TRB_setCtrlBit(setupStage, 	XHCI_TRB_Ctrl_idt);
		HW_USB_XHCI_TRB_setTRT(setupStage, 		XHCI_TRB_TRT_No);
	}
	{
		HW_USB_XHCI_TRB_setDir(&req->trb[1], 		((~dir) & 1));
		HW_USB_XHCI_TRB_setType(&req->trb[1], 		XHCI_TRB_Type_StatusStage);
		HW_USB_XHCI_TRB_setCtrlBit(&req->trb[1], 	XHCI_TRB_Ctrl_ioc);
	}
}

void HW_USB_XHCI_ctrlDataReq(XHCI_Request *req, u64 setup, int dir, void *data, u16 len) {
	{
		XHCI_GenerTRB *setupStage = &req->trb[0];
		HW_USB_XHCI_TRB_setData(setupStage, 	setup);
		HW_USB_XHCI_TRB_setStatus(setupStage, 	HW_USB_XHCI_TRB_mkStatus(0x08, 0, 0));
		HW_USB_XHCI_TRB_setType(setupStage, 	XHCI_TRB_Type_SetupStage);
		HW_USB_XHCI_TRB_setCtrlBit(setupStage, 	XHCI_TRB_Ctrl_idt);
		HW_USB_XHCI_TRB_setTRT(setupStage, 		dir == XHCI_TRB_Ctrl_Dir_In ? XHCI_TRB_TRT_In : XHCI_TRB_TRT_Out);
	}
	{
		XHCI_GenerTRB *dataStage = &req->trb[1];
		HW_USB_XHCI_TRB_setData(dataStage,		DMAS_virt2Phys(data));
		HW_USB_XHCI_TRB_setStatus(dataStage, 	HW_USB_XHCI_TRB_mkStatus(len, 0, 0));
		HW_USB_XHCI_TRB_setType(dataStage, 		XHCI_TRB_Type_DataStage);
		HW_USB_XHCI_TRB_setDir(dataStage, 		dir);
	}
	{
		HW_USB_XHCI_TRB_setDir(&req->trb[2], 		((~dir) & 1));
		HW_USB_XHCI_TRB_setType(&req->trb[2], 		XHCI_TRB_Type_StatusStage);
		HW_USB_XHCI_TRB_setCtrlBit(&req->trb[2], 	XHCI_TRB_Ctrl_ioc);
	}
}

XHCI_Request *HW_USB_XHCI_allocReq(u64 trbCnt) {
	XHCI_Request *req = kmalloc(sizeof(XHCI_Request), Slab_Flag_Clear | Slab_Flag_Private, (void *)HW_USB_XHCI_freeReq);
	req->trb = kmalloc(sizeof(XHCI_GenerTRB) * trbCnt, Slab_Flag_Clear, NULL);
	req->target = kmalloc(sizeof(XHCI_Request **) * trbCnt, Slab_Flag_Clear, NULL);
	req->trbCnt = trbCnt;
	return req;
}

// wait for the result of request and return the completion code
int HW_USB_XHCI_Req_wait(XHCI_Request *req) {
	while (!(req->flags & XHCI_Request_Flag_Finished)) IO_hlt();
	return HW_USB_XHCI_TRB_getCmplCode(&req->res);
}

int HW_USB_XHCI_Req_ringDbWait(XHCI_Host *host, u32 slotId, u32 epId, u32 taskId, XHCI_Request *req) {
	HW_USB_XHCI_writeDbReg(host, slotId, epId, taskId);
	return HW_USB_XHCI_Req_wait(req);
}

void HW_USB_XHCI_writeCtx(void *ctx, int dwId, u32 mask, u32 val) {
	HW_USB_XHCI_writeDword((u64)ctx + dwId * sizeof(u32), (HW_USB_XHCI_readDword((u64)ctx + dwId * sizeof(u32)) & ~mask) | (val << (Bit_ffs(mask) - 1)));
}

u32 HW_USB_XHCI_readCtx(void *ctx, int dwId, u32 mask) {
	return (HW_USB_XHCI_readDword((u64)ctx + dwId * sizeof(u32)) & mask) >> (Bit_ffs(mask) - 1);
}

u32 HW_USB_XHCI_EpCtx_readMxESITPay(XHCI_EpCtx *ep) {
	return (HW_USB_XHCI_readCtx(ep, 0, XHCI_EpCtx_mxESITPayH) << 16) | HW_USB_XHCI_readCtx(ep, 4, XHCI_EpCtx_mxESITPayL);
}

void HW_USB_XHCI_EpCtx_writeMxESITPay(XHCI_EpCtx *ep, u32 val) {
	HW_USB_XHCI_writeCtx(ep, 0, XHCI_EpCtx_mxESITPayH, (val >> 16) & 0xffu);
	HW_USB_XHCI_writeCtx(ep, 4, XHCI_EpCtx_mxESITPayL, val & 0xffffu);
}

u32 HW_USB_XHCI_EpCtx_getMxPackSize0(u32 speed) {
	switch (speed) {
		case XHCI_Port_Speed_Super :	return 512;
		case XHCI_Port_Speed_Low : 		return 8;
		case XHCI_Port_Speed_Full : 	
		case XHCI_Port_Speed_High : 	return 64;
	}
	return 512;
}