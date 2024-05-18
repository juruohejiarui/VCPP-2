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
        if (cachePool[cachePoolSize - 1] == NULL) {
            printk(RED, BLACK, "PageTable_alloc(): fail to allocate a page for page table\n");
            return 0;
        }
        cacheSize += 0x1000;
    }
    memset(DMAS_phys2Virt(page->phyAddr), 0, 512 * sizeof(u64));
    return page->phyAddr;
}

void PageTable_map2M(u64 cr3, u64 vAddr, u64 pAddr) {
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = PageTable_alloc() | 0x7;
    u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
    if (*pmdEntry == 0) *pmdEntry = pAddr | 0x86 | (pAddr > 0);
    flushTLB();
}

void PageTable_init() {
    cachePool[(cachePoolSize = 1) - 1] = Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel);
    cacheSize = 0x1000;
    // unmap the 0-th entry of pgd
    u64 cr3 = getCR3();
    u64 *pgd = (u64 *)DMAS_phys2Virt(cr3);
    pgd[0] = 0;
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
void PageTable_map(u64 cr3, u64 vAddr, u64 pAddr) {
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
	// if (pAddr > 0) printk(YELLOW, BLACK, "cr3: %#018lx ", cr3);
	// if (pAddr > 0) printk(YELLOW, BLACK, "(try map %#018lx -> %#018lx) ", vAddr, pAddr);
    if (*entry == 0) *entry = PageTable_alloc() | 0x7;
	// if (pAddr > 0) printk(WHITE, BLACK, "->%#018lx ", *entry);
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
    if (*entry == 0) *entry = PageTable_alloc() | 0x7;
	// if (pAddr > 0) printk(WHITE, BLACK, "->%#018lx ", *entry);
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
    if (*entry == 0) *entry = PageTable_alloc() | 0x7;
	// if (pAddr > 0) printk(WHITE, BLACK, "->%#018lx ", *entry);
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 12) & 0x1ff);
    *entry = pAddr | 0x6 | (pAddr > 0);
	// if (pAddr > 0) printk(WHITE, BLACK, "->%#018lx ", *entry);
	// if (pAddr != 0) printk(BLUE, BLACK, "Map %#018lx -> %#018lx", pAddr, vAddr);
	// if (pAddr > 0) printk(WHITE, BLACK, "...\n");
	flushTLB();
}

u64 PageTable_getPldEntry(u64 cr3, u64 vAddr) {
	printk(YELLOW, BLACK, "PageTable_getPldEntry(): cr3 = %#018lx, vAddr = %#018lx\n", cr3, vAddr);
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
	printk(WHITE, BLACK, "entry = %#018lx", *entry);
    if (*entry == 0) return 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 30) & 0x1ff);
	printk(WHITE, BLACK, "->%#018lx\t", *entry);
    if (*entry == 0) return 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 21) & 0x1ff);
	printk(WHITE, BLACK, "->%#018lx\t", *entry);
    if (*entry == 0) return 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xfff) + ((vAddr >> 12) & 0x1ff);
	printk(WHITE, BLACK, "->%#018lx\n", *entry);
    return *entry;
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