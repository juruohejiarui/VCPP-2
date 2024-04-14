#include "buddy.h"
#include "DMAS.h"
#include "../includes/log.h"

#define Buddy_maxOrder 15

static inline int Page_getOrder(Page *pageStructAddr) {
    return (pageStructAddr->attr >> 6) & ((1ul << 4) - 1);
}
static inline int Page_setOrder(Page *page, int ord) {
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
#define buddyPages(headPage) (isLeft(headPage->buddyId) ? (headPage + (1 << Page_getOrder(headPage))) : (headPage - (1 << Page_getOrder(headPage))))

static inline int getBit(Page *headPage) {
    if (headPage->buddyId == 1) return 1;
    u64 pos = headPage->phyAddr >> Page_4KShift;
    pos -= isRight(headPage->buddyId) * (1 << Page_getOrder(headPage));
    return (mmStruct.bitmap[Page_getOrder(headPage)][pos / 64] >> (pos % 64)) & 1;
}

static inline void revBit(Page *headPage) {
    if (headPage->buddyId == 1) return ;
    u64 pos = headPage->phyAddr >> Page_4KShift;
    pos -= isRight(headPage->buddyId) * (1 << Page_getOrder(headPage));
    mmStruct.bitmap[Page_getOrder(headPage)][pos / 64] ^= (1ul << (pos % 64));
}


static inline void insNewFreePageFrame(int ord, Page *headPage) {
    List_init(&headPage->listEle);
    if (mmStruct.freeList[ord] == NULL) mmStruct.freeList[ord] = headPage;
    else List_insBefore(&headPage->listEle, &mmStruct.freeList[ord]->listEle);
}

static inline Page *popFreePageFrame(int ord) {
    Page *headPage = mmStruct.freeList[ord];
    if (headPage == NULL) return NULL;
    if (List_isEmpty(&headPage->listEle)) mmStruct.freeList[ord] = NULL;
    else List_del(&headPage->listEle), List_init(&headPage->listEle);
    return headPage;
}

void Buddy_init() {
    // allocate pages for bitmap
    printk(RED, BLACK, "Buddy_init()\n");
    u64 bitsSize = upAlignTo(Page_4KUpAlign(memManageStruct.totMemSize) >> Page_4KShift, 64) / 8;
    if (Page_4KSize > bitsSize) {
        u64 numOfOnePage = (u64)Page_4KSize / bitsSize;
        printk(GREEN, BLACK, "One page for %d bitmap\n", numOfOnePage);
        for (u64 i = 0; i <= Buddy_maxOrder; i += numOfOnePage) {
            Page *page = BsMemManage_alloc(1, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            for (int j = 0; j < numOfOnePage  && i + j < Buddy_maxOrder; j++)
                mmStruct.bitmap[i + j] = DMAS_phys2Virt(page->phyAddr + bitsSize * j);
        }
    } else {
        u64 regPage = Page_4KUpAlign(bitsSize) / Page_4KSize;
        printk(GREEN, BLACK, "%d pages for one bitmap\n", regPage);
        for (int i = 0; i <= Buddy_maxOrder; i++) {
            Page *page = BsMemManage_alloc(regPage, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            mmStruct.bitmap[i] = DMAS_phys2Virt(page->phyAddr);
        }
    }
    for (int i = 0; i <= Buddy_maxOrder; i++)
        memset(mmStruct.bitmap[i], 0, bitsSize), mmStruct.freeList[i] = NULL;
    // initialize the free list
    for (int i = 1; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        if (zone->usingCnt == zone->pagesLength) continue;
        u64 pgPos = zone->usingCnt;
        for (int ord = Buddy_maxOrder; ord >= 0; ord--)
            while (pgPos + (1 << ord) <= zone->pagesLength) {
                Page *headPage = zone->pages + pgPos;
                headPage->attr |= Page_Flag_BuddyHeadPage;
                Page_setOrder(headPage, ord);
                List_init(&headPage->listEle);
                headPage->buddyId = 1;
                insNewFreePageFrame(ord, headPage);
                pgPos += (1 << ord);
            }
    }
}

Page *Buddy_alloc(u64 log2Size, u64 attr) {
    if (log2Size > Buddy_maxOrder) return NULL;
    for (int ord = log2Size; ord <= Buddy_maxOrder; ord++) {
        Page *headPage = popFreePageFrame(ord);
        if (headPage == NULL) continue;
        // divide this page frame
        for (int i = ord; i > log2Size; i--) {
            Page *rPage = headPage + (1 << (i - 1));
            rPage->buddyId = rChildPos(headPage->buddyId);
            headPage->buddyId = lChildPos(headPage->buddyId);
            Page_setOrder(rPage, i - 1);
            Page_setOrder(headPage, i - 1);
            rPage->attr |= Page_Flag_BuddyHeadPage;
            insNewFreePageFrame(i - 1, rPage);
            revBit(rPage);
        }
        headPage->attr |= attr;
        return headPage;
    }
    return NULL;
}

void Buddy_free(Page *pages) {
    if (pages == NULL || (pages->attr & Page_Flag_BuddyHeadPage) == 0) return;
    for (int i = Page_getOrder(pages); i < Buddy_maxOrder; i++) {
        revBit(pages);
        if (getBit(pages)) break;
        Page *buddy = buddyPages(pages);
        // the buddy page is the the only free page in freeList[i]
        if (List_isEmpty(&buddy->listEle))
            mmStruct.freeList[i] = NULL;
        List_del(&buddy->listEle), List_init(&buddy->listEle);
        Page    *rChild = isRight(pages->buddyId) ? pages : buddy,
                *lChild = isRight(pages->buddyId) ? buddy : pages;
        rChild->attr = 0;
        lChild->attr = Page_Flag_BuddyHeadPage;
        rChild->buddyId = 0,        lChild->buddyId = parentPos(lChild->buddyId);
        Page_setOrder(rChild, 0),   Page_setOrder(lChild, i + 1);
        pages = lChild;
    }
    insNewFreePageFrame(Page_getOrder(pages), pages);
}

void Buddy_debugLog(int range) {
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