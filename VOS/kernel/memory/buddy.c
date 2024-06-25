#include "buddy.h"
#include "DMAS.h"
#include "../includes/log.h"

#define Buddy_maxOrder 15

inline int MM_Buddy_getOrder(Page *pageStructAddr) {
    return (pageStructAddr->attr >> 6) & ((1ul << 4) - 1);
}
inline void MM_Buddy_setOrder(Page *page, int ord) {
    page->attr = (page->attr & (~(((1ul << 4) - 1) << 6))) | (ord << 6);
}

static struct BuddyManageStruct {
    Page *freeList[Buddy_maxOrder + 1];
    u64 *bitmap[Buddy_maxOrder + 1];
} mmStruct;

#define parentPos(pos) ((pos) >> 1)
#define lChildPos(pos) ((pos) << 1)
#define rChildPos(pos) (((pos) << 1) | 1)
#define isLeft(pos) (!((pos) & 1))
#define isRight(pos) ((pos) & 1)
#define buddyPages(headPage) (isLeft(headPage->buddyId) ? (headPage + (1 << MM_Buddy_getOrder(headPage))) : (headPage - (1 << MM_Buddy_getOrder(headPage))))

static int getBit(Page *headPage) {
    if (headPage->buddyId == 1) return 1;
    u64 pos = headPage->phyAddr >> Page_4KShift, ord = MM_Buddy_getOrder(headPage);
    pos -= isRight(headPage->buddyId) * (1 << ord);
    return (mmStruct.bitmap[ord][pos / 64] >> (pos % 64)) & 1;
}

static void revBit(Page *headPage) {
    if (headPage->buddyId == 1) return ;
    u64 pos = headPage->phyAddr >> Page_4KShift, ord = MM_Buddy_getOrder(headPage);
    pos -= isRight(headPage->buddyId) * (1 << ord);
    mmStruct.bitmap[ord][pos / 64] ^= (1ul << (pos % 64));
}


static inline void _insNewFreePageFrame(int ord, Page *headPage) {
    List_init(&headPage->listEle);
    if (mmStruct.freeList[ord] == NULL) mmStruct.freeList[ord] = headPage;
    else List_insBefore(&headPage->listEle, &mmStruct.freeList[ord]->listEle);
}

static inline Page *_popFreePageFrame(int ord) {
    Page *headPage = mmStruct.freeList[ord];
    if (headPage == NULL) return NULL;
    if (List_isEmpty(&headPage->listEle)) mmStruct.freeList[ord] = NULL;
    else mmStruct.freeList[ord] = container(headPage->listEle.next, Page, listEle), List_del(&headPage->listEle);
    return headPage;
}

void MM_Buddy_init() {
    // allocate pages for bitmap
    printk(RED, BLACK, "MM_Buddy_init()\n");
    u64 bitsSize = upAlignTo(Page_4KUpAlign(memManageStruct.totMemSize) >> Page_4KShift, 64) / 8;
    if (Page_4KSize > bitsSize) {
        u64 numOfOnePage = (u64)Page_4KSize / bitsSize;
		#ifdef DEBUG_MM
        printk(GREEN, BLACK, "One page for %d bitmap\n", numOfOnePage);
		#endif
        for (u64 i = 0; i <= Buddy_maxOrder; i += numOfOnePage) {
            Page *page = MM_Bs_alloc(1, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "MM_Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            for (int j = 0; j < numOfOnePage  && i + j < Buddy_maxOrder; j++)
                mmStruct.bitmap[i + j] = DMAS_phys2Virt(page->phyAddr + bitsSize * j);
        }
    } else {
        u64 regPage = Page_4KUpAlign(bitsSize) / Page_4KSize;
		#ifdef DEBUG_MM
        printk(GREEN, BLACK, "%d pages for one bitmap\n", regPage);
		#endif
        for (int i = 0; i <= Buddy_maxOrder; i++) {
            Page *page = MM_Bs_alloc(regPage, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "MM_Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            mmStruct.bitmap[i] = DMAS_phys2Virt(page->phyAddr);
        }
    }
    for (int i = 0; i <= Buddy_maxOrder; i++)
        memset(mmStruct.bitmap[i], 0, bitsSize), mmStruct.freeList[i] = NULL;
    memManageStruct.totMemSize = 0;
    // initialize the free list
    for (int i = 1; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        if (zone->usingCnt == zone->pagesLength) continue;
        u64 pgPos = zone->usingCnt;
		while (pgPos < zone->pagesLength) {
			Page *headPage = zone->pages + pgPos;
			u64 ord = min(log2(lowbit(headPage->phyAddr)) - 12, Buddy_maxOrder);
			while (pgPos + (1ul << ord) > zone->pagesLength) ord--;
			headPage->attr |= Page_Flag_BuddyHeadPage;
			MM_Buddy_setOrder(headPage, ord);
			List_init(&headPage->listEle);
			headPage->buddyId = 1;
			_insNewFreePageFrame(ord, headPage);
			pgPos += (1ul << ord);
			memManageStruct.totMemSize += (1 << (ord + Page_4KShift));
		}
    }
}

static inline void _divPageFrame(Page *page, int fr, int to) {
	for (int i = fr; i > to; i--) {
		Page *rPage = page + (1 << (i - 1));
		rPage->buddyId = rChildPos(page->buddyId);
		page->buddyId = lChildPos(page->buddyId);
		MM_Buddy_setOrder(rPage, i - 1);
		MM_Buddy_setOrder(page, i - 1);
		rPage->attr |= Page_Flag_BuddyHeadPage;
		_insNewFreePageFrame(i - 1, rPage);
		revBit(rPage);
	}
}

Page *MM_Buddy_alloc(u64 log2Size, u64 attr) {
    IO_Func_maskIntrPreffix
    if (log2Size > Buddy_maxOrder) return NULL;
    for (int ord = log2Size; ord <= Buddy_maxOrder; ord++) {
        Page *headPage = _popFreePageFrame(ord);
        if (headPage == NULL) continue;
        // divide this page frame
        _divPageFrame(headPage, ord, log2Size);
        headPage->attr |= attr;
		IO_Func_maskIntrSuffix
		#ifdef DEBUG_MM_ALLOC
        printk(GREEN, BLACK, "MM_Buddy_alloc(%d)->%p [%#018lx,%#018lx]\t", log2Size, headPage, headPage->phyAddr, headPage->phyAddr + (1 << (log2Size + Page_4KShift)) - 1);
		#endif
		if (log2(lowbit(headPage->phyAddr)) - 12 < log2Size) {
			printk(RED, BLACK, "Buddy Align Error: %d->%d\n", log2(lowbit(headPage->phyAddr)) - 12, log2Size);
			while (1) IO_hlt();
		}
        return headPage;
    }
    IO_Func_maskIntrSuffix
    return NULL;
}

Page *MM_Buddy_alloc4G(u64 log2Size, u64 attr) {
	IO_Func_maskIntrPreffix
	if (log2Size > Buddy_maxOrder) return NULL;
	for (int ord = log2Size; ord <= Buddy_maxOrder; ord++) {
		Page *page = mmStruct.freeList[ord];
		if (page == NULL) continue;
		// find the page below 4G
		do {
			if (page->phyAddr < (1ul << 32)) break;
			page = container(page->listEle.next, Page, listEle);
		} while (page != mmStruct.freeList[ord]);
		if (page->phyAddr >= (1ul << 32)) continue;
		// remove this page from free list
		if (page == mmStruct.freeList[ord]) {
			if (List_isEmpty(&page->listEle)) mmStruct.freeList[ord] = NULL;
			else mmStruct.freeList[ord] = container(page->listEle.next, Page, listEle);
		}
		List_del(&page->listEle);
		// divide this page frame
		_divPageFrame(page, ord, log2Size);
		page->attr |= attr;

		IO_Func_maskIntrSuffix
		return page;
	}
	IO_Func_maskIntrSuffix
	return NULL;
}

void MM_Buddy_free(Page *pages) {
    IO_Func_maskIntrPreffix
	#ifdef DEBUG_MM_ALLOC
    printk(RED, BLACK, "MM_Buddy_free(%p)\n", pages);
	#endif
    if (pages == NULL || (pages->attr & Page_Flag_BuddyHeadPage) == 0) return;
    List_del(&pages->listEle);
    pages->attr = Page_Flag_BuddyHeadPage;
    for (int i = MM_Buddy_getOrder(pages); i < Buddy_maxOrder; i++) {
        revBit(pages);
        if (getBit(pages)) break;
        Page *buddy = buddyPages(pages);
        // the buddy page is the the only free page in freeList[i]
        if (List_isEmpty(&buddy->listEle))
            mmStruct.freeList[i] = NULL;
        List_del(&buddy->listEle);
        Page    *rChild = isRight(pages->buddyId) ? pages : buddy,
                *lChild = isRight(pages->buddyId) ? buddy : pages;
        rChild->attr = 0;
        lChild->attr = Page_Flag_BuddyHeadPage;
        rChild->buddyId = 0,        lChild->buddyId = parentPos(lChild->buddyId);
        MM_Buddy_setOrder(rChild, 0),   MM_Buddy_setOrder(lChild, i + 1);
        pages = lChild;
    }
    _insNewFreePageFrame(MM_Buddy_getOrder(pages), pages);
    IO_Func_maskIntrSuffix
}

void MM_Buddy_debugLog(int range) {
    for (int i = 0; i <= min(Buddy_maxOrder, range); i++) {
        printk(ORANGE, BLACK, "mmStruct.freeList[%d] = %p\n", i, mmStruct.freeList[i]);
        if (mmStruct.freeList[i] != NULL) {
            Page *page = mmStruct.freeList[i];
            int cnt = 0;
            do {
                printk(WHITE, BLACK, "[%#018lx, %#018lx]%c", page, page->phyAddr, (((++cnt) % 5 == 0) ? '\n' : ' '));
                page = container(page->listEle.next, Page, listEle);
            } while (page != mmStruct.freeList[i]);
            putchar(WHITE, BLACK, '\n');
        }
    }
}

Page *MM_Buddy_divPageFrame(Page *headPage) {
    int ord = MM_Buddy_getOrder(headPage);
    if (ord == 0) return NULL;
    Page *rPage = headPage + (1 << (ord - 1));
    rPage->buddyId = rChildPos(headPage->buddyId);
    headPage->buddyId = lChildPos(headPage->buddyId);
    MM_Buddy_setOrder(rPage, ord - 1);
    MM_Buddy_setOrder(headPage, ord - 1);
    List_init(&rPage->listEle);
    rPage->attr = headPage->attr;
    return rPage;
}
