#include "SLAB.h"
#include "DMAS.h"
#include "buddy.h"
#include "pgtable.h"
#include "../includes/log.h"

#define alloc2MPage() Buddy_alloc(9, Page_Flag_Kernel)

/// @brief Create a slab cache struct and one slab struct
/// @param size the size of slabs
/// @param constructer pointer of constucter function
/// @param destructor pointer of destucter function
/// @param arg some other args
/// @return the pointer to the slab cache struct
SlabCache *Slab_createCache(u64 size, 
        void *(*constructer)(void *virtAddr, u64 arg), void *(*destructor)(void *virtAddr, u64 arg),
        u64 arg) {
    SlabCache *cache = (SlabCache *)kmalloc(sizeof(SlabCache), 0);
    if (cache == NULL) {
        printk(RED, BLACK, "Slab_createCache: kmalloc failed\n");
        return NULL;
    }
    memset(cache, 0, sizeof(SlabCache));
    cache->size = upAlignTo(size, sizeof(u64));
    cache->usingCnt = cache->freeCnt = 0;
    cache->slabs = (Slab *)kmalloc(sizeof(Slab), 0);
    if (cache->slabs == NULL) {
        printk(RED, BLACK, "Slab_createCache: kmalloc failed\n");
        kfree(cache);
        return NULL;
    }
    memset(cache->slabs, 0, sizeof(Slab));

    cache->constructer = constructer;
    cache->destructor = destructor;
    List_init(&cache->slabs->listEle);

    cache->slabs->page = alloc2MPage(); // Allocate 2MB
    if (cache->slabs->page == NULL) {
        printk(RED, BLACK, "Slab_createCache: Buddy_alloc failed\n");
        kfree(cache->slabs);
        kfree(cache);
        return NULL;
    }
    cache->slabs->usingCnt = 0, cache->slabs->freeCnt = Page_2MSize / cache->size;
    cache->freeCnt = cache->slabs->freeCnt;
    // map it to DMAS address area
    cache->slabs->virtAddr = DMAS_phys2Virt(cache->slabs->page->phyAddr);

    cache->slabs->colCnt = cache->slabs->freeCnt;
    cache->slabs->colLen = upAlignTo(cache->slabs->colCnt, sizeof(u64)) / 8;
    cache->slabs->colMap = (u64 *)kmalloc(cache->slabs->colLen * sizeof(u64), 0);
    if (cache->slabs->colMap == NULL) {
        printk(RED, BLACK, "Slab_createCache: kmalloc failed\n");
        Buddy_free(cache->slabs->page);
        kfree(cache->slabs);
        kfree(cache);
        return NULL;
    }
    memset(cache->slabs->colMap, 0, cache->slabs->colLen * sizeof(u64));
    
    return cache;
}

/// @brief Destroy a slab cache struct and all slabs
/// @param cache the pointer to the slab cache struct
/// @return 1: success, 0: failed
u64 Slab_destroy(SlabCache *cache) {
    if (cache == NULL) return 0;
    Slab *slabs = cache->slabs, *tmp = NULL;
    if (slabs->usingCnt > 0) {
        printk(RED, BLACK, "Slab_destroy: some slabs are still in using\n");
        return 0;
    }
    while (!List_isEmpty(&slabs->listEle)) {
        tmp = slabs;
        slabs = container(slabs->listEle.next, Slab, listEle);
        List_del(&tmp->listEle);
        kfree(tmp->colMap);
        Buddy_free(tmp->page);
        kfree(tmp);
    }
    kfree(slabs->colMap);
    Buddy_free(slabs->page);
    kfree(slabs);
    kfree(cache);
    return 1;
}

/// @brief Allocate a memory block from slab cache
/// @param cache 
/// @param arg 
/// @return the pointer to the memory block
void *Slab_malloc(SlabCache *cache, u64 arg) {
    Slab *slab = cache->slabs;
    // extend the slab
    if (cache->freeCnt == 0) {
        Slab *newSlab = (Slab *)kmalloc(sizeof(Slab), 0);
        if (newSlab == NULL) {
            printk(RED, BLACK, "Slab_malloc: kmalloc failed\n");
            return NULL;
        }
        memset(newSlab, 0, sizeof(Slab));
        List_init(&newSlab->listEle);
        newSlab->page = alloc2MPage(); // Allocate 2MB
        if (newSlab->page == NULL) {
            printk(RED, BLACK, "Slab_malloc: Buddy_alloc failed\n");
            kfree(newSlab);
            return NULL;
        }
        newSlab->usingCnt = 0, newSlab->freeCnt = Page_2MSize / cache->size;
        cache->freeCnt += newSlab->freeCnt;
        // map it to DMAS address area
        newSlab->virtAddr = DMAS_phys2Virt(newSlab->page->phyAddr);

        newSlab->colCnt = newSlab->freeCnt;
        newSlab->colLen = upAlignTo(newSlab->colCnt, sizeof(u64)) / 8;
        newSlab->colMap = (u64 *)kmalloc(newSlab->colLen * sizeof(u64), 0);
        if (newSlab->colMap == NULL) {
            printk(RED, BLACK, "Slab_malloc: kmalloc failed\n");
            Buddy_free(newSlab->page);
            kfree(newSlab);
            return NULL;
        }
        memset(newSlab->colMap, 0, newSlab->colLen * sizeof(u64));

        List_insBefore(&newSlab->listEle, &slab->listEle);
        for (int j = 0; j < newSlab->colCnt; j++) {
            if (newSlab->colMap[j >> 6] & (1ul << (j & 63))) continue;
            Bit_set(newSlab->colMap + (j >> 6), j & 63);
            cache->freeCnt--;
            cache->usingCnt++;
            newSlab->freeCnt--;
            newSlab->usingCnt++;
            if (cache->constructer != NULL) 
                return cache->constructer((char *)newSlab->virtAddr + j * cache->size, arg);
            else return (void *)((char *)newSlab->virtAddr + j * cache->size);
        }
        List_del(&newSlab->listEle);
        kfree(newSlab->colMap);
        Buddy_free(newSlab->page);
        kfree(newSlab);
    } else { // find a free block in the slab
        do {
            if (slab->freeCnt == 0) {
                slab = container(slab->listEle.next, Slab, listEle);
                continue;
            }
            for (int j = 0; j < slab->colCnt; j++) {
                if (slab->colMap[j >> 6] & (1ul << (j & 63))) continue;
                Bit_set(slab->colMap + (j >> 6), j & 63);
                cache->freeCnt--;
                cache->usingCnt++;
                slab->freeCnt--;
                slab->usingCnt++;
                if (cache->constructer != NULL) 
                    return cache->constructer((char *)slab->virtAddr + j * cache->size, arg);
                else return (void *)((char *)slab->virtAddr + j * cache->size);
            }
        } while (slab != cache->slabs);
    }
    printk(RED, BLACK, "Slab_malloc: no free block\n");
    return NULL;
}

u64 Slab_free(SlabCache *cache, void *addr, u64 arg) {
    Slab *slab = cache->slabs;
    int id = 0;
    do {
        // the address is in this slab
        if (slab->virtAddr <= addr && addr < (void *)((char *)slab->virtAddr + Page_2MSize)) {
            id = ((char *)addr - (char *)slab->virtAddr) / cache->size;
            Bit_clear(slab->colMap + (id >> 6), id & 63);
            cache->freeCnt++;
            cache->usingCnt--;
            slab->freeCnt++;
            slab->usingCnt--;
            if (cache->destructor != NULL) cache->destructor(addr, arg);
            // free the slab if it is not used and freeCnt is enough
            if (slab->usingCnt == 0 && cache->freeCnt >= slab->freeCnt * 3 / 2) {
                List_del(&slab->listEle);
                cache->freeCnt -= slab->freeCnt;
                kfree(slab->colMap);
                Buddy_free(slab->page);
                kfree(slab);
            }
            return 1;
        } else
            slab = container(slab->listEle.next, Slab, listEle); // next slab
    } while (slab != cache->slabs);
    printk(RED, BLACK, "Slab_free: address not in the slab\n");
    return 0;
}

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

u64 Slab_init() {
    // calculate the total size of Slab and colMap
    
}