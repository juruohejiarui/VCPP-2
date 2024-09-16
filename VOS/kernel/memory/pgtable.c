#include "pgtable.h"
#include "buddy.h"
#include "DMAS.h"
#include "../includes/task.h"
#include "../includes/log.h"
#include "../includes/task.h"

#define PGTable_maxCacheSize 0x1100
#define PGTable_minCacheSize 0x100

__always_inline__ int _getPldIndex(u64 vAddr) { return ((vAddr >> 12) & 0x1ff); }
__always_inline__ int _getPmdIndex(u64 vAddr) { return ((vAddr >> 21) & 0x1ff); }
__always_inline__ int _getPudIndex(u64 vAddr) { return ((vAddr >> 30) & 0x1ff); }
__always_inline__ int _getPgdIndex(u64 vAddr) { return ((vAddr >> 39) & 0x1ff); }

static SpinLock _PageTableLocker;

static Page *cachePool[PGTable_maxCacheSize];
static int cachePoolSize = 0, cacheSize;

u64 MM_PageTable_alloc() {
    IO_maskIntrPreffix
	SpinLock_lock(&_PageTableLocker);
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
        cachePool[cachePoolSize++] = MM_Buddy_alloc(11, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelShare);
        if (cachePool[cachePoolSize - 1] == NULL) {
            printk(RED, BLACK, "MM_PageTable_alloc(): fail to allocate a page for page table\n");
            return (u64)NULL;
        }
        cacheSize += (1 << 11);
    }
	SpinLock_unlock(&_PageTableLocker);
    IO_maskIntrSuffix
    memset(DMAS_phys2Virt(page->phyAddr), 0, 512 * sizeof(u64));
	page->attr |= Page_Flag_MMU;
    return page->phyAddr;
}

u64 MM_PageTable_free(u64 *tbl) {
    IO_maskIntrPreffix
    SpinLock_lock(&_PageTableLocker);
    Page *page = NULL;
	for (int i = 0; i < memManageStruct.zonesLength; i++) {
		Zone *zone = &memManageStruct.zones[i];
		if (zone->phyAddrSt <= DMAS_virt2Phys(tbl) && DMAS_virt2Phys(tbl) < zone->phyAddrEd) {
			page = zone->pages + (DMAS_virt2Phys(tbl) - zone->phyAddrSt) / Page_4KSize;
			break;
		}
	}
	if (!page || !(page->attr & Page_Flag_MMU)) {
		printk(RED, BLACK, "MM_PageTable: %#018lx is not a page table\n", tbl);
		while (1) IO_hlt();
		return (u64)-1;
	}
    // if there is enough cache, then just free this page
    if (cachePoolSize == PGTable_maxCacheSize)
        MM_Buddy_free(page);
    else {
        cachePool[cachePoolSize++] = page;
        cacheSize++;
    }
    SpinLock_unlock(&_PageTableLocker);
    IO_maskIntrSuffix
}

void MM_PageTable_map1G(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
	vAddr = Page_1GDownAlign(vAddr), pAddr = Page_1GDownAlign(pAddr);
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = MM_PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = pAddr | 0x80 | flag | (pAddr > 0);
	flushTLB();
}

void MM_PageTable_map2M(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
	vAddr = Page_2MDownAlign(vAddr), pAddr = Page_2MDownAlign(pAddr);
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + ((vAddr >> 39) & 0x1ff);
    if (*pgdEntry == 0) *pgdEntry = MM_PageTable_alloc() | 0x7;
    u64 *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xffful) + ((vAddr >> 30) & 0x1ff);
    if (*pudEntry == 0) *pudEntry = MM_PageTable_alloc() | 0x7;
	else if (*pudEntry & 0x80) return ;
    u64 *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xffful) + ((vAddr >> 21) & 0x1ff);
    if (*pmdEntry == 0) *pmdEntry = pAddr | 0x80 | flag | (pAddr > 0);
    flushTLB();
}

void MM_PageTable_cleanTmpMap() {
    u64 cr3 = getCR3();
    u64 *pgd = (u64 *)DMAS_phys2Virt(cr3);

    pgd[0] = 0;
	flushTLB();
}

void MM_PageTable_init() {
	SpinLock_init(&_PageTableLocker);
    cachePool[0] = MM_Buddy_alloc(12, Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelShare);
    cacheSize = 0x1000, cachePoolSize = 1;
    // unmap the 0-th entry of pgd
    

	// map all the space not in zones
	for (int i = 0; i <= memManageStruct.e820Length; i++) {
		if (memManageStruct.e820[i].type == 1) continue;
		u64 bound = Page_4KUpAlign(memManageStruct.e820[i].addr + memManageStruct.e820[i].size);
		for (u64 addr = Page_4KDownAlign(memManageStruct.e820[i].addr); addr < bound;) {
			if (!(addr & ((1ul << Page_1GShift) - 1)) && addr + Page_1GSize <= bound)
				MM_PageTable_map1G(getCR3(), (u64)DMAS_phys2Virt(addr), addr, MM_PageTable_Flag_Writable),
				addr += Page_1GSize;
			else if (!(addr * ((1ul << Page_2MShift) - 1)) && addr + Page_2MSize <= bound)
				MM_PageTable_map2M(getCR3(), (u64)DMAS_phys2Virt(addr), addr, MM_PageTable_Flag_Writable),
				addr += Page_2MSize;
			else 
				MM_PageTable_map(getCR3(), (u64)DMAS_phys2Virt(addr), addr, MM_PageTable_Flag_Writable | MM_PageTable_Flag_Presented),
				addr += Page_4KSize;
		}
	}
	flushTLB();
}

/// @brief Map a memory block [pAddr, pAddr + 4K - 1]
/// @param vAddr
/// @param pAddr
void MM_PageTable_map(u64 cr3, u64 vAddr, u64 pAddr, u64 flag) {
	vAddr = Page_4KDownAlign(vAddr), pAddr = Page_4KDownAlign(pAddr);
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + _getPgdIndex(vAddr);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPudIndex(vAddr);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
	if (*entry & 0x80) return ;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPmdIndex(vAddr);
    if (*entry == 0) *entry = MM_PageTable_alloc() | 0x7;
	if (*entry & 0x80) return ;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPldIndex(vAddr);
    *entry = pAddr | flag;
}

u64 MM_PageTable_getPldEntry(u64 cr3, u64 vAddr) {
    u64 *entry = (u64 *)DMAS_phys2Virt(cr3) + _getPgdIndex(vAddr);
    if (*entry == 0) return 0;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPudIndex(vAddr);
    if (*entry == 0) return 0;
	if (*entry & 0x80) return *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPmdIndex(vAddr);
    if (*entry == 0) return 0;
	if (*entry & 0x80) return *entry;
    entry = (u64 *)DMAS_phys2Virt(*entry & ~0xffful) + _getPldIndex(vAddr);
    return *entry;
}

// return 1 if free this table successfully, return 0 otherwise.
int _tryFree(u64 *tbl) {
    int empty = 1;
    for (int i = 0; i < 0x200; i++) if (tbl[i] != (u64)NULL) { empty = 1; break; }
    if (!empty) return 0;
    MM_PageTable_free(tbl);
    return 1;
}

void MM_PageTable_unmap(u64 cr3, u64 vAddr) {
    u64 *pgdEntry = (u64 *)DMAS_phys2Virt(cr3) + _getPgdIndex(vAddr),
        *pudEntry = (u64 *)DMAS_phys2Virt(*pgdEntry & ~0xffful) + _getPudIndex(vAddr),
        *pmdEntry = (u64 *)DMAS_phys2Virt(*pudEntry & ~0xffful) + _getPmdIndex(vAddr),
        *pldEntry = (u64 *)DMAS_phys2Virt(*pmdEntry & ~0xffful) + _getPldIndex(vAddr);
    if ((*pldEntry & ~0xffful) != (u64)NULL)
    *pldEntry = 0;
    // try to free the pld table
    int succ = _tryFree((u64 *)(*pmdEntry & ~0xffful));
    if (!succ) return ;
    *pmdEntry = 0;
    // try to free the pmd table
    succ = _tryFree((u64 *)(*pudEntry & ~0xffful));
    if (!succ) return ;
    // try to free the pud table
    *pldEntry = 0;
    succ = _tryFree((u64 *)(*pgdEntry & ~0xffful));
    if (succ) *pgdEntry = 0;
}

// free all the physical memory and the page table entries
// lvl: 0: pldTable, 1: pmdTable, 2: pudTable
void _cleanMap(u64 *entry, int lvl) {
    if (!lvl) {
        MM_PageTable_free(entry);
        return ;
    }
    for (u64 i = 0; i < 0x200; i++) {
        if ((entry[i] & ~0xffful) == 0) continue;
        _cleanMap(DMAS_phys2Virt(entry[i] & ~0xffful), lvl - 1);
    }
    MM_PageTable_free(entry);
}
void MM_PageTable_cleanMap(u64 cr3) {
    u64 *pgdEntry = DMAS_phys2Virt(cr3), i;
    for (i = 0; i < 256; i++) {
        if ((pgdEntry[i] & ~0xffful) == 0) continue;
        _cleanMap(DMAS_phys2Virt(pgdEntry[i] & ~0xffful), 2);
    }
    if (pgdEntry[0x1ff] & ~0xffful)
        _cleanMap(DMAS_phys2Virt(pgdEntry[0x1ff] & ~0xffful), 2);
    MM_PageTable_free(pgdEntry);
}