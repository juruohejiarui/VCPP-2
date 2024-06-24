#include "pgtable.h"
#include "buddy.h"
#include "DMAS.h"
#include "../includes/task.h"
#include "../includes/log.h"

#define PGTable_maxCacheSize 0x1100
#define PGTable_minCacheSize 0x100

static Page *cachePool[PGTable_maxCacheSize];
static int cachePoolSize = 0, cacheSize;

u64 MM_PageTable_alloc() {
    IO_Func_maskIntrPreffix
    // find a page for page table
    Page *page = cachePool[cachePoolSize - 1];
    if (MM_Buddy_getOrder(page) == 0) cachePool[--cachePoolSize] = NULL;
    else {
        cachePoolSize--;
        while (MM_Buddy_getOrder(page) > 0)
            cachePool[cachePoolSize++] = MM_Buddy_divPageFrame(page);
    }
    cacheSize--;
    if (cacheSize < PGTable_minCacheSize) {
        cachePool[cachePoolSize++] = MM_Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
        if (cachePool[cachePoolSize - 1] == NULL) {
            printk(RED, BLACK, "MM_PageTable_alloc(): fail to allocate a page for page table\n");
            return (u64)NULL;
        }
        cacheSize += 0x1000;
    }
    memset(DMAS_phys2Virt(page->phyAddr), 0, 512 * sizeof(u64));
    IO_Func_maskIntrSuffix
    return page->phyAddr;
}

void MM_PageTable_map1G(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
    printk(WHITE, BLACK, "MM_PageTable_map1G(): cr3:%#018lx vAddr:%#018lx pAddr:%#018lx flag:%#018lx\n", cr3, vAddr, pAddr, flag);
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = MM_PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    *pudEntry = pAddr | 0x80 | flag | (pAddr > 0);
}

void MM_PageTable_map2M(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = MM_PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = MM_PageTable_alloc() | 0x7;
    u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xffful) + ((vAddr >> 21) & 0x1ff);
    if (*pmdEntry == 0) *pmdEntry = pAddr | 0x80 | flag | (pAddr > 0);
    flushTLB();
}

void MM_PageTable_init() {
    cachePool[0] = MM_Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
    cacheSize = 0x1000, cachePoolSize = 1;
    // unmap the 0-th entry of pgd
    u64 cr3 = getCR3();
    u64 *pgd = (u64 *)DMAS_phys2Virt(cr3);
    pgd[0] = 0;
	flushTLB();
}

void PGTable_free(u64 phyAddr) {
    Page *page = memManageStruct.pages + (phyAddr >> Page_4KShift);
    if (cacheSize >= PGTable_maxCacheSize) MM_Buddy_free(page);
    else {
        cachePool[cachePoolSize++] = page;
        cacheSize++;
    }
}

/// @brief Map a memory block [pAddr, pAddr + 4K - 1]
/// @param vAddr
/// @param pAddr
void MM_PageTable_map(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 21) & 0x1ff);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 12) & 0x1ff);
    *entry = pAddr | flag;
	flushTLB();
}

u64 MM_PageTable_getPldEntry(u64 cr3, u64 vAddr) {
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*entry == 0) return 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    if (*entry == 0) return 0;
	if (*entry & 0x80) return *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 21) & 0x1ff);
    if (*entry == 0) return 0;
	if (*entry & 0x80) return *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 12) & 0x1ff);
    return *entry;
}

u64 MM_PageTable_getPldEntry_debug(u64 cr3, u64 vAddr) {
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    printk(WHITE, BLACK, "MM_PageTable_getPldEntry_debug: cr3:%#018lx->%#018lx", cr3, *entry);
    if (*entry == 0) return printk(WHITE, BLACK, "\n"), 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    printk(WHITE, BLACK, "->%#018lx", *entry);
    if (*entry == 0) return printk(WHITE, BLACK, "\n"), 0;
	if (*entry & 0x80) return printk(WHITE, BLACK, "\n"), *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 21) & 0x1ff);
    printk(WHITE, BLACK, "->%#018lx", *entry);
    if (*entry == 0) return printk(WHITE, BLACK, "\n"), 0;
	if (*entry & 0x80) return printk(WHITE, BLACK, "\n"), *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + ((vAddr >> 12) & 0x1ff);
    printk(WHITE, BLACK, "->%#018lx\n", *entry);
    return *entry;
}


/// @brief fork the current page table, using the allocated physics address in the old page table
/// @return the physics address of the new page table
u64 MM_PageTable_fork() {
    u64 oldCr3 = getCR3(), newCR3;
    Page *pgdPage = MM_Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
    newCR3 = pgdPage->phyAddr;
    u64 *oldPgd = (u64 *)DMAS_phys2Virt(oldCr3), *newPgd = (u64 *)DMAS_phys2Virt(pgdPage->phyAddr);
    memset(newPgd, 0, 512 * sizeof(u64));
    for (int i = 0; i < 512; i++) if (*(oldPgd + i)) {
        Page *pudPage = MM_Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
        *(newPgd + i) = pudPage->phyAddr | 0x7;
        u64 *oldPud = (u64 *)DMAS_phys2Virt(*(oldPgd + i)), *newPud = (u64 *)DMAS_phys2Virt(pudPage->phyAddr);
        memset(newPud, 0, 512 * sizeof(u64));
        for (int j = 0; j < 512; j++) if (*(oldPud + j)) {
            Page *pmdPage = MM_Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
            *(newPud + j) = pmdPage->phyAddr | 0x7;
            u64 *oldPmd = (u64 *)DMAS_phys2Virt(*(oldPud + j)), *newPmd = (u64 *)DMAS_phys2Virt(pmdPage->phyAddr);
            memset(newPmd, 0, 512 * sizeof(u64));
            for (int k = 0; k < 512; k++) if (*(oldPmd + k)) {
                Page *pldPage = MM_Buddy_alloc(0, Page_Flag_Active | Page_Flag_Kernel);
                *(newPmd + k) = pldPage->phyAddr | 0x7;
                u64 *oldPld = (u64 *)DMAS_phys2Virt(*(oldPmd + k)), *newPld = (u64 *)DMAS_phys2Virt(pldPage->phyAddr);
                memcpy(newPld, oldPld, 512 * sizeof(u64));
            }
        }
    }
    return newCR3;
}