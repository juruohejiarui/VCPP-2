#include "pgtable.h"
#include "buddy.h"
#include "DMAS.h"

#define PGTable_maxCacheSize 0x1100
#define PGTable_minCacheSize 0x100

static Page *cachePool[PGTable_maxCacheSize];
static int cachePoolSize = 0, cacheSize;

void PageTable_init() {
    cachePool[cachePoolSize++] = Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
    cacheSize = 0x1000;
}

u64 PageTable_alloc() {
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

/// @brief Map a memory block [pAddr, pAddr + 4K - 1]
/// @param vAddr
/// @param pAddr
void PageTable_map(u64 vAddr, u64 pAddr) {
    u64 cr3 = getCR3();
    if (vAddr >= kernelAddrStart) {
        // map the kernel space
        u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
        if (*pgdEntry == 0) *pgdEntry = PageTable_alloc() | 0x7;
        u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
        if (*pudEntry == 0) *pudEntry = PageTable_alloc() | 0x7;
        u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
        if (*pmdEntry == 0) *pmdEntry = PageTable_alloc() | 0x7;
        u64 *pldEntry = (u64 *)DMAS_phys2Virt(*pmdEntry & ~0xfff) + ((vAddr >> 12) & 0x1ff);
        *pldEntry = pAddr | 0x7;
    }
}
