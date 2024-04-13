#include "buddy.h"
#include "DMAS.h"
#include "../includes/log.h"

#define Buddy_maxOrder 15
static struct BuddyManageStruct {
    Page *freeList[Buddy_maxOrder + 1];
    u64 *bitmap[Buddy_maxOrder + 1];
} mmStruct;

#define getParentPos(pos) ((pos) >> 1)
#define getLeftChildPos(pos) ((pos) << 1)
#define getRightChildPos(pos) (((pos) << 1) | 1)
#define isLeft(pos) (!((pos) & 1))
#define isRight(pos) ((pos) & 1)

static inline void revBit(Page *headPage) {
    u64 pos = headPage->phyAddr >> Page_4KShift;
    pos -= isRight(headPage->buddyId) * (1 << Page_getOrder(headPage));
    mmStruct.bitmap[Page_getOrder(headPage)][pos / 64] ^= (1ul << (pos % 64));
}
static inline int getBit(Page *headPage) {
    u64 pos = headPage->phyAddr >> Page_4KShift;
    pos -= isRight(headPage->buddyId) * (1 << Page_getOrder(headPage));
    return (mmStruct.bitmap[Page_getOrder(headPage)][pos / 64] >> (pos % 64)) & 1;
}

void Buddy_init() {
    // allocate pages for bitmap
    printk(RED, BLACK, "Buddy_init()\n");
    printk(WHITE, BLACK, "memManageStruct.bitsSize = %d\n", memManageStruct.bitsSize);
    if (Page_4KSize > memManageStruct.bitsSize) {
        u64 numOfOnePage = (u64)Page_4KSize / memManageStruct.bitsSize;
        printk(GREEN, BLACK, "One page for %d bitmap\n", numOfOnePage);
        for (u64 i = 0; i < Buddy_maxOrder; i += numOfOnePage) {
            Page *page = BsMemManage_alloc(1, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            for (int j = 0; j < numOfOnePage  && i + j < Buddy_maxOrder; j++)
                mmStruct.bitmap[i + j] = DMAS_phys2Virt(page->phyAddr + memManageStruct.bitsSize * j);
        }
    } else {
        u64 regPage = Page_4KUpAlign(memManageStruct.bitsSize) / Page_4KSize;
        printk(GREEN, BLACK, "%d pages for one bitmap\n", regPage);
        for (int i = 0; i < Buddy_maxOrder; i++) {
            Page *page = BsMemManage_alloc(regPage, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelInit);
            if (page == NULL) {
                printk(RED, BLACK, "Buddy_init() failed to allocate memory for bitmap\n");
                return;
            }
            mmStruct.bitmap[i] = DMAS_phys2Virt(page->phyAddr);
        }
    }
    // log: allocate memory for bitmap
    for (int i = 0; i <= Buddy_maxOrder; i++)
        printk(WHITE, BLACK, "mmStruct.bitmap[%d] = %p\n", i, mmStruct.bitmap[i]);
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
                if (mmStruct.freeList[ord] == NULL) mmStruct.freeList[ord] = headPage;
                else List_insBefore(&headPage->listEle, &mmStruct.freeList[ord]->listEle);
                pgPos += (1 << ord);
            }
    }
    // log: free list
    for (int i = 0; i <= Buddy_maxOrder; i++) {
        printk(ORANGE, BLACK, "mmStruct.freeList[%d] = %p\n", i, mmStruct.freeList[i]);
        if (mmStruct.freeList[i] != NULL) {
            Page *page = mmStruct.freeList[i];
            do {
                printk(WHITE, BLACK, "%#018lx, phyAddr = %#018lx, attr = %lx, buddyId = %d\n", page, page->phyAddr, page->attr, page->buddyId);
                page = container(page->listEle.next, Page, listEle);
            } while (page != mmStruct.freeList[i]);
        }
    }
}