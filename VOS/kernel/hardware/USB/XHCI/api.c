#include "api.h"
#include "../../../includes/log.h"

void HW_USB_XHCI_TRB_copy(XHCI_GenerTRB *src, XHCI_GenerTRB *dst) {
	HW_USB_XHCI_writeQuad((u64)dst, *(u64 *)src);
	HW_USB_XHCI_writeQuad((u64)dst + 0x10, *(u64 *)&src->status);
}

u64 HW_USB_XHCI_readQuad(u64 addr) {
	return HW_USB_XHCI_readDword(addr) | (((u64)HW_USB_XHCI_readDword(addr + 0x4)) << 32);
}

u32 HW_USB_XHCI_readDword(u64 addr) { 
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

void HW_USB_XHCI_writeDword(u64 addr, u32 val) {
	__asm__ volatile (
		"movl %0, (%1)		\n\t"
		"mfence				\n\t"
		:
		: "a"(val), "b"(addr)
		: "memory"
	);
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
	const u32 mask = (1 << 5) - 1;
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
	kfree(ring, 0);
}

XHCI_Ring *HW_USB_XHCI_allocRing(u64 size) {
	XHCI_Ring *ring = kmalloc(sizeof(XHCI_Ring), Slab_kmalloc_arg_Clear, NULL);
	ring->ring = kmalloc(sizeof(XHCI_GenerTRB) * size, Slab_kmalloc_arg_Clear, NULL);
	ring->reqSrc = kmalloc(sizeof(XHCI_Request *) * size, Slab_kmalloc_arg_Clear, NULL);
	ring->cur = ring->ring;
	ring->cycBit = 1;
	SpinLock_init(&ring->lock);
	return ring;
}


void HW_USB_XHCI_freeEveRing(XHCI_EveRing *ring) {
	for (int i = 0; i < ring->ringNum; i++)
		kfree(ring->rings[i], 0);
	kfree(ring->rings, 0);
	kfree(ring, 0);
}

XHCI_EveRing *HW_USB_XHCI_allocEveRing(u32 num, u32 size) {
	XHCI_EveRing *ring = kmalloc(sizeof(XHCI_EveRing), Slab_kmalloc_arg_Clear, NULL);
	ring->ringNum = num, ring->ringSize = size;
	ring->cycBit = 1;
	ring->curRingId = ring->curPos = 0;
	ring->rings = kmalloc(sizeof(XHCI_GenerTRB *) * num, Slab_kmalloc_arg_Clear, NULL);
	for (int i = 0; i < num; i++) ring->rings[i] = kmalloc(size * sizeof(XHCI_GenerTRB), Slab_kmalloc_arg_Clear, NULL);
	return ring;
}

void HW_USB_XHCI_freeReq(XHCI_Request *req) {
	if (req->trb) kfree(req->trb, 0);
	if (req->target) kfree(req->target, 0);
	kfree(req, 0);
}

XHCI_Request *HW_USB_XHCI_allocReq(u64 trbCnt) {
	XHCI_Request *req = kmalloc(sizeof(XHCI_Request), Slab_kmalloc_arg_Clear, NULL);
	req->trb = kmalloc(sizeof(XHCI_GenerTRB) * trbCnt, Slab_kmalloc_arg_Clear, NULL);
	req->target = kmalloc(sizeof(XHCI_Request *) * trbCnt, Slab_kmalloc_arg_Clear, NULL);
	req->trbCnt = trbCnt;
	List_init(&req->list);
	return req;
}

int HW_USB_XHCI_Ring_tryInsReq(XHCI_Ring *ring, XHCI_Request *req) {
	SpinLock_lock(&ring->lock);
	static u64 pos[XHCI_Ring_maxSize], cyc[XHCI_Ring_maxSize];
	int trbC = 0, full = 0;
	XHCI_GenerTRB *lstCur = ring->cur;
	int lstPos = ring->curPos, lstCyc = ring->cycBit;
	for (int i = 0; i < req->trbCnt; i++) {
		if (HW_USB_XHCI_TRB_getType(ring->cur) == XHCI_TRB_Type_Link) {
			pos[trbC] = ring->curPos, cyc[trbC++] = ring->cycBit;
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
		pos[trbC] = ring->curPos, cyc[trbC++] = ring->cycBit;
	}
	if (full) {
		// restore and return fail code
		ring->cur = lstCur, ring->curPos = lstPos, ring->cycBit = lstCyc;
		SpinLock_unlock(&ring->lock);
		return 0;
	}
	for (int i = 0, reqP = 0; i < trbC; i++) {
		XHCI_GenerTRB *trb = &ring->ring[pos[i]];
		if (HW_USB_XHCI_TRB_getType(trb) != XHCI_TRB_Type_Link) {
			HW_USB_XHCI_TRB_copy(&req->trb[reqP], trb);
			ring->reqSrc[pos[i]] = req;
			reqP++;
		}
		HW_USB_XHCI_TRB_setCycBit(trb, cyc[i]);
		printk(ORANGE, BLACK, "%#018lx cyc:%d\n", trb, cyc[i]);
	}
	SpinLock_unlock(&ring->lock);
	return 1;
}

int HW_USB_XHCI_EveRing_getNxt(XHCI_EveRing *ring, XHCI_GenerTRB **trb) {
	XHCI_GenerTRB *tmp = &ring->rings[ring->curRingId][ring->curPos];
	if (HW_USB_XHCI_TRB_getCycBit(tmp) != ring->cycBit) return 0;
	*trb = tmp;
	if ((++ring->curPos) == ring->ringSize) {
		if ((++ring->curRingId) == ring->ringNum) {
			ring->curRingId = 0;
			ring->cycBit ^= 1;
		}
		ring->curPos = 0;
	}
	return 1;
}
