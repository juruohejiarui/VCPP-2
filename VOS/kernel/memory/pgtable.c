#include "pgtable.h"
#include "buddy.h"
#include "DMAS.h"
#include "../includes/task.h"
#include "../includes/log.h"

#define PGTable_maxCacheSize 0x1100
#define PGTable_minCacheSize 0x100

static Page *cachePool[PGTable_maxCacheSize];
static int cachePoolSize = 0, cacheSize;

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

void PageTable_map2M(u64 vAddr, u64 pAddr) {
    u64 cr3 = getCR3();
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = PageTable_alloc() | 0x7;
    u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
    if (*pmdEntry == 0) *pmdEntry = pAddr | 0x87;
    flushTLB();
}

void PageTable_init() {
    cachePool[cachePoolSize++] = Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
    cacheSize = 0x1000;
    // unmap the 0-th entry of pgd
    u64 cr3 = getCR3();
    u64 *pgd = (u64 *)DMAS_phys2Virt(cr3);
    pgd[0] = 0;
    // mapping the address [0x200000, 0x100000000] to [0xffff800000200000, 0xffff800100000000]
    for (u64 pAddr = 0x200000; pAddr < 0x100000000; pAddr += 0x200000)
        PageTable_map2M(pAddr + 0xffff800000000000, pAddr);
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
    printk(BLUE, BLACK, "PageTable_map: vAddr = %#018lx, pAddr = %#018lx\n", vAddr, pAddr);
    u64 cr3 = getCR3();
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = PageTable_alloc() | 0x7;
    u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
    if (*pmdEntry == 0) *pmdEntry = PageTable_alloc() | 0x7;
    u64 *pldEntry = (u64 *)DMAS_phys2Virt(*pmdEntry & ~0xfff) + ((vAddr >> 12) & 0x1ff);
    *pldEntry = pAddr | 0x7;
    flushTLB();
}

/// @brief fork the current page table, using the allocated physics address in the old page table
/// @return the physics address of the new page table
u64 PageTable_fork() {
    u64 oldCr3 = getCR3(), newCR3;
    Page *pgdPage = Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
    newCR3 = pgdPage->phyAddr;
    u64 *oldPgd = (u64 *)DMAS_phys2Virt(oldCr3), *newPgd = (u64 *)DMAS_phys2Virt(pgdPage->phyAddr);
    memset(newPgd, 0, 512 * sizeof(u64));
    for (int i = 0; i < 512; i++) if (*(oldPgd + i)) {
        Page *pudPage = Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
        *(newPgd + i) = pudPage->phyAddr | 0x7;
        u64 *oldPud = (u64 *)DMAS_phys2Virt(*(oldPgd + i)), *newPud = (u64 *)DMAS_phys2Virt(pudPage->phyAddr);
        memset(newPud, 0, 512 * sizeof(u64));
        for (int j = 0; j < 512; j++) if (*(oldPud + j)) {
            Page *pmdPage = Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
            *(newPud + j) = pmdPage->phyAddr | 0x7;
            u64 *oldPmd = (u64 *)DMAS_phys2Virt(*(oldPud + j)), *newPmd = (u64 *)DMAS_phys2Virt(pmdPage->phyAddr);
            memset(newPmd, 0, 512 * sizeof(u64));
            for (int k = 0; k < 512; k++) if (*(oldPmd + k)) {
                Page *pldPage = Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
                *(newPmd + k) = pldPage->phyAddr | 0x7;
                u64 *oldPld = (u64 *)DMAS_phys2Virt(*(oldPmd + k)), *newPld = (u64 *)DMAS_phys2Virt(pldPage->phyAddr);
                memcpy(newPld, oldPld, 512 * sizeof(u64));
            }
        }
    }
    return newCR3;
}