#include "pgtable.h"
#include "buddy.h"
#include "DMAS.h"

#define PGTable_maxCacheSize 0x1100
#define PGTable_minCacheSize 0x100

static Page *cachePool[PGTable_maxCacheSize];
static int cachePoolSize = 0, cacheSize;

void PGTable_init() {
    cachePool[cachePoolSize++] = Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
    cacheSize = 0x1000;
}

u64 PGTable_alloc() {
    // find a page for page table
    Page *page = cachePool[cachePoolSize - 1];
    if (Page_getOrder(page) == 0) cachePool[--cachePoolSize] = NULL;
    else {
        cachePoolSize--;
        while (Page_getOrder(page) > 0)
            cachePool[cachePoolSize++] = Buddy_dividePageFrame(page);
    }
    cacheSize--;
    if (cacheSize < PGTable_minCacheSize) {
        cachePool[cachePoolSize++] = Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
        cacheSize += 0x1000;
    }
    memset(DMAS_phys2Virt(page->phyAddr), 0, 512 * sizeof(u64));
    return page->phyAddr;
}

void PGTable_free(u64 phyAddr) {
    Page *page = memManageStruct.pages + (phyAddr >> Page_4KShift);
    if (cacheSize >= PGTable_maxCacheSize) Buddy_free(page);
    else {
        cachePool[cachePoolSize++] = page;
        cacheSize++;
    }
}

void PageTable_map(u64 vAddr, u64 pAddr) {
    u64 cr3 = getCR3();
}
