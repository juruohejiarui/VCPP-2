#include "SLAB.h"
#include "DMAS.h"
#include "buddy.h"
#include "pgtable.h"
#include "../includes/log.h"
#include "../includes/task.h"

#define alloc2MPage() MM_Buddy_alloc(9, Page_Flag_Kernel | Page_Flag_KernelShare)

static SpinLock _SlabLocker;

SlabCache Slab_kmallocCache[16] = {
    {32,        0, 0, NULL, NULL, NULL},
    {64,        0, 0, NULL, NULL, NULL},
    {128,       0, 0, NULL, NULL, NULL},
    {256,       0, 0, NULL, NULL, NULL},
    {512,       0, 0, NULL, NULL, NULL},
    {1024,      0, 0, NULL, NULL, NULL}, // 1KB
    {2048,      0, 0, NULL, NULL, NULL},
    {4096,      0, 0, NULL, NULL, NULL},
    {8192,      0, 0, NULL, NULL, NULL},
    {16384,     0, 0, NULL, NULL, NULL},
    {32768,     0, 0, NULL, NULL, NULL},
    {65536,     0, 0, NULL, NULL, NULL},
    {131072,    0, 0, NULL, NULL, NULL},
    {262144,    0, 0, NULL, NULL, NULL},
    {524288,    0, 0, NULL, NULL, NULL},
    {1048576,   0, 0, NULL, NULL, NULL} // 1MB
};

void _addUsage(void *addr, void (*destructor)(void *), u64 size) {
	Task_KmallocUsage *usage = kmalloc(sizeof(Task_KmallocUsage), 0, NULL);
	List_init(&usage->listEle);
	List_insBefore(&usage->listEle, &Task_current->mem->kmallocUsage);
	usage->addr = addr;
	usage->destructor = destructor;
	usage->size = size;
	Task_current->mem->totUsage += size;
}

void _delUsage(void *addr) {
	for (List *usageList = Task_current->mem->kmallocUsage.next; usageList != &Task_current->mem->kmallocUsage; usageList = usageList->next) {
		Task_KmallocUsage *usage = container(usageList, Task_KmallocUsage, listEle);
		if (usage->addr != addr) continue;
		if (usage->destructor) usage->destructor(addr);
		Task_current->mem->totUsage -= usage->size;
		List_del(usageList);
		kfree(usage, 0);
		return ;
	}
	printk(RED, BLACK, "kfree: %#018lx is not private memory\n", addr);
	while (1) IO_hlt();
}

void MM_Slab_init() {
    printk(RED, BLACK, "MM_Slab_init()\n");
	SpinLock_init(&_SlabLocker);
    // calculate the total size of Slab and colMap
    u64 totSize = 16 * sizeof(Slab);
    for (int i = 0; i < 16; i++)
        totSize += upAlignTo(Page_2MSize / Slab_kmallocCache[i].size, 64) / 64 * sizeof(u64);
    totSize = Page_4KUpAlign(totSize);
	#ifdef DEBUG_MM
    printk(GREEN, BLACK, "MM_Slab_init: require %d Bytes for initialization\n", totSize);
	#endif
    // up align to power of 2
    u64 log2Size = 0;
    while ((1ul << log2Size) < (totSize >> Page_4KShift)) log2Size++;

	#ifdef DEBUG_MM
    printk(GREEN, BLACK, "MM_Slab_init: require %d Pages for initialization\n", 1ul << log2Size);
	#endif

    Page *page = MM_Buddy_alloc(log2Size, Page_Flag_Kernel | Page_Flag_KernelShare);
    if (page == NULL) {
        printk(RED, BLACK, "MM_Slab_init: MM_Buddy_alloc for slab root failed\n");
		while (1) IO_hlt();
        return ;
    }
    u64 virtAddr = (u64)DMAS_phys2Virt(page->phyAddr);
    for (int i = 0; i < 16; i++) {
        Slab_kmallocCache[i].slabs = (Slab *)virtAddr;
        virtAddr += sizeof(Slab);
        List_init(&Slab_kmallocCache[i].slabs->listEle);

        // initial the information for the first slab of this cache
        Slab_kmallocCache[i].slabs->page = alloc2MPage();
        if (Slab_kmallocCache[i].slabs->page == NULL) {
            printk(RED, BLACK, "MM_Slab_init: MM_Buddy_alloc() for slabs failed\n");
            MM_Buddy_free(page);
			while (1) IO_hlt();
            return ;
        }
        Slab_kmallocCache[i].slabs->usingCnt = 0, Slab_kmallocCache[i].slabs->freeCnt = Page_2MSize / Slab_kmallocCache[i].size;
        Slab_kmallocCache[i].freeCnt = Slab_kmallocCache[i].slabs->freeCnt;
        Slab_kmallocCache[i].slabs->virtAddr = DMAS_phys2Virt(Slab_kmallocCache[i].slabs->page->phyAddr);
        Slab_kmallocCache[i].slabs->colMap = (u64 *)virtAddr;
        Slab_kmallocCache[i].slabs->colCnt = Slab_kmallocCache[i].slabs->freeCnt;
        Slab_kmallocCache[i].slabs->colLen = upAlignTo(Slab_kmallocCache[i].slabs->colCnt, 64) / 64;
        memset(Slab_kmallocCache[i].slabs->colMap, 0xff, Slab_kmallocCache[i].slabs->colLen * sizeof(u64));
        virtAddr += Slab_kmallocCache[i].slabs->colLen * sizeof(u64);
        for (int j = 0; j < Slab_kmallocCache[i].slabs->colCnt; j++) 
            Bit_set0(Slab_kmallocCache[i].slabs->colMap + (j >> 6), j & 63);
    }
}

void MM_Slab_debugLog() {
    for (int i = 0; i < 16; i++) {
        printk(ORANGE, BLACK, "Slab_cache[%d]: size = %d, usingCnt = %d, freeCnt = %d\n", 
            i, Slab_kmallocCache[i].size, Slab_kmallocCache[i].usingCnt, Slab_kmallocCache[i].freeCnt);
        Slab *slab = Slab_kmallocCache[i].slabs;
        do {
            printk(WHITE, BLACK, "\t(%p): usingCnt = %06d, freeCnt = %06d, colMap[0] = %#018lx, virtAddr = %#018lx\n", 
                slab, slab->usingCnt, slab->freeCnt, slab->colMap[0], slab->virtAddr);
            slab = container(slab->listEle.next, Slab, listEle);
        } while (slab != Slab_kmallocCache[i].slabs);
    }
}

void Slab_pushNewSlab(int id) {
    Page *page2M = alloc2MPage();
    Slab *slab;
    if (page2M == NULL) {
        printk(RED, BLACK, "Slab_pushNewSlab: MM_Buddy_alloc failed\n");
        return ;
    }
    u64 structSize = 0; void *virtAddr;
    // 32, 64, 128, 256, 512
    if (0 <= id && id < 5) {
        virtAddr = DMAS_phys2Virt(page2M->phyAddr);
        structSize = sizeof(Slab) + upAlignTo(Page_2MSize / Slab_kmallocCache[id].size, 64) / 64 * sizeof(u64);
        // set the address of struct and the colmap to the end of this page
        slab = (Slab *)((char *)virtAddr + Page_2MSize - structSize);
        slab->usingCnt = 0;
        slab->freeCnt = (Page_2MSize - structSize) / Slab_kmallocCache[id].size;

        slab->colCnt = slab->freeCnt;
        slab->colLen = upAlignTo(slab->colCnt, 64) / 64;
        slab->colMap = (u64 *)((char *)slab + sizeof(Slab));

        slab->virtAddr = DMAS_phys2Virt(page2M->phyAddr);
        slab->page = page2M;
    } else { // 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB, 128KB, 256KB, 512KB, 1MB
        slab = (Slab *)kmalloc(sizeof(Slab), Slab_Flag_Inner, NULL);
        slab->usingCnt = 0;
        slab->freeCnt = Page_2MSize / Slab_kmallocCache[id].size;

        slab->colCnt = slab->freeCnt;
        slab->colLen = upAlignTo(slab->colCnt, 64) / 64;
        slab->colMap = (u64 *)kmalloc(slab->colLen * sizeof(u64), Slab_Flag_Inner, NULL);

        slab->virtAddr = DMAS_phys2Virt(page2M->phyAddr);
        slab->page = page2M;
    }
    memset(slab->colMap, 0xff, slab->colLen * sizeof(u64));
    for (int j = 0; j < slab->colCnt; j++) 
        Bit_set0(slab->colMap + (j >> 6), j & 63);

    List_init(&slab->listEle);
    List_insBehind(&slab->listEle, &Slab_kmallocCache[id].slabs->listEle);
    Slab_kmallocCache[id].freeCnt += slab->freeCnt;
}

/// @brief allocate a memory block for kernel process from the slab system
/// @param size the size of memory block
/// @param arg the argument for this allocation, bit 0 : from inner code, will not acquire spin lock; bit 1 : privated memory block
/// @return the pointer to the memory block
void *kmalloc(u64 size, u64 arg, void (*destructor)(void *)) {
    IO_maskIntrPreffix
    // printk(BLACK, WHITE, "kmalloc %08d\t", size);
	if (!(arg & Slab_Flag_Inner)) SpinLock_lock(&_SlabLocker);
    int id = 0;

    if (size > MM_Slab_maxSize) {
		if (!(arg & Slab_Flag_Inner)) SpinLock_unlock(&_SlabLocker);
		return NULL;
	}

    while (Slab_kmallocCache[id].size < size) id++;
    Slab *slab = NULL;
    // find a slab with free memory block
    if (Slab_kmallocCache[id].freeCnt > 0) {
        slab = Slab_kmallocCache[id].slabs;
        do {
            if (slab->freeCnt > 0) break;
            slab = container(slab->listEle.next, Slab, listEle);
        } while (slab != Slab_kmallocCache[id].slabs);
    } else { // create a new slab
        Slab_pushNewSlab(id);
        slab = container(Slab_kmallocCache[id].slabs->listEle.next, Slab, listEle);
    }

    // find a free memory block
    for (u64 j = 0; j < slab->colCnt; j++) {
        if (slab->colMap[j >> 6] == 0xfffffffffffffffful) {j += 63; continue; }
        if (Bit_get(slab->colMap + (j >> 6), j & 63) != 0) continue;
        Bit_set1(slab->colMap + (j >> 6), j & 63);
        slab->usingCnt++, slab->freeCnt--;
        Slab_kmallocCache[id].usingCnt++, Slab_kmallocCache[id].freeCnt--;
		
		// finally calculate the address
		u64 addr = (u64)slab->virtAddr + j * Slab_kmallocCache[id].size;
		// printk(WHITE, BLACK, "size=%d: %#018lx + %d*%#018x=%#018lx\n", size, slab->virtAddr, j, Slab_kmallocCache[id].size, addr);
		if (!(arg & Slab_Flag_Inner)) SpinLock_unlock(&_SlabLocker);
        IO_maskIntrSuffix
		// clear the memory block to 0 if needed
		if (arg & Slab_Flag_Clear) memset((void *)addr, 0, size);
		// release the spin lock and enable the interrupts
		// record this memory block if it is private
		if (arg & Slab_Flag_Private) _addUsage((void *)addr, destructor, Slab_kmallocCache[id].size); 
        return (void *)(addr);
    }
    printk(RED, BLACK, "kmalloc: invalid state\n");

	if (!(arg & Slab_Flag_Inner)) SpinLock_unlock(&_SlabLocker);
    IO_maskIntrSuffix

    return NULL;
}

void Slab_destroySlab(int id, Slab *slab) {
    List_del(&slab->listEle);
    Slab_kmallocCache[id].freeCnt -= slab->freeCnt;
    if (0 <= id && id < 5) {
        MM_Buddy_free(slab->page);
    } else {
        kfree(slab->colMap, Slab_Flag_Inner);
        MM_Buddy_free(slab->page);
        kfree(slab, Slab_Flag_Inner);
    }
}

void kfree(void *addr, u64 arg) {
	if (arg & Slab_Flag_Private) _delUsage(addr);
    IO_maskIntrPreffix
    if (!(arg & Slab_Flag_Inner)) SpinLock_lock(&_SlabLocker);
    int id = 0, flag = 0;
    Slab *slab = NULL;
    for (id = 0; id < 16; id++) {
        slab = Slab_kmallocCache[id].slabs;
        do {
            if (slab->virtAddr <= addr && addr < (void *)((u64)slab->virtAddr + slab->colCnt * Slab_kmallocCache[id].size)) {
                flag = 1;
                break;
            }
            slab = container(slab->listEle.next, Slab, listEle);
        } while (slab != Slab_kmallocCache[id].slabs);
        if (flag) break;
    }
    if (!flag) {
        printk(RED, BLACK, "kfree: invalid address %#018lx\n", addr);
        while (1) ;
        if (!(arg & Slab_Flag_Inner)) SpinLock_unlock(&_SlabLocker);
        IO_maskIntrSuffix
        return ;
    }
    u64 offset = ((u64)addr - (u64)slab->virtAddr) / Slab_kmallocCache[id].size;
    Bit_set0(slab->colMap + (offset >> 6), offset & 63);
    slab->freeCnt++, slab->usingCnt--;
    Slab_kmallocCache[id].freeCnt++, Slab_kmallocCache[id].usingCnt--;
    if (slab->usingCnt == 0 && Slab_kmallocCache[id].freeCnt >= slab->colCnt * 3 / 2 && Slab_kmallocCache[id].slabs != slab)
        Slab_destroySlab(id, slab);

    if (!(arg & Slab_Flag_Inner)) SpinLock_unlock(&_SlabLocker);
    IO_maskIntrSuffix
}