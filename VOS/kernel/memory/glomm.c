#include "../includes/memory.h"
#include "../includes/UEFI.h"
#include "../includes/printk.h"

struct GlobalMemoryDescriptor memManageStruct = {{0}, 0};

int dmaIndex = 0;
int normalIndex = 0;    // below 1GB RAM, was mapped in pagetable
int unmapIndex = 0;     // above 1GB RAM, was not mapped in pagetable

u64 *globalCR3;

// update the page table and flush the TLB (Translation Lookaside Buffer)
#define flushTLB() \
do { \
    u64 tmpreg; \
    __asm__ __volatile__( \
        "movq %%cr3, %0\n\t" \
        "movq %0, %%cr3\n\t" \
        :"=r"(tmpreg) \
        : \
        : "memory"); \
} while(0)

u64 initPage(Page *page, u64 flags) {
    printk(GREEN, BLACK, "page %#018lx flag = %#018lx\n", page, flags);
    if (!page->attribute) {
        *(memManageStruct.bitmap + ((page->phyAddr >> PAGE_2M_SHIFT) >> 6)) ^= 1ul << ((page->phyAddr >> PAGE_2M_SHIFT) % 64);
        page->attribute = flags;
        page->refCnt++;
        page->blgZone->pageFreeCnt--;
        page->blgZone->pageUsingCnt++;
        page->blgZone->totPageLink++;
    } else if ((page->attribute || flags) & (PAGE_Referred || PAGE_Shared_K2U)) {
        page->attribute |= flags;
        page->refCnt++;
        page->blgZone->totPageLink++;
    } else {
        *(memManageStruct.bitmap + ((page->phyAddr >> PAGE_2M_SHIFT) >> 6)) ^= 1ul << ((page->phyAddr >> PAGE_2M_SHIFT) % 64);
        page->attribute |= flags;
    }
    return 0;
}

void initMemory() {
    memManageStruct.codeSt = (u64)&_text;
    memManageStruct.codeEd = (u64)&_etext;
    memManageStruct.dataEd = (u64)&_edata;
    memManageStruct.brkEd  = (u64)&_end;

    EFI_E820MemoryDescriptor *p = (EFI_E820MemoryDescriptor *)bootParamInfo->E820Info.entry;
    u64 totMem = 0;
    printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");

    for (int i = 0; i < bootParamInfo->E820Info.entryCount; i++) {
        printk(GREEN, BLACK, "UEFI=>KERN---Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        if (p->type == 1) totMem += p->length;
        memManageStruct.e820[i].address = p->address;
        memManageStruct.e820[i].length = p->length;
        memManageStruct.e820[i].type = p->type;

        memManageStruct.e820Size = i;
        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1) break;
    }
    printk(WHITE, BLACK, "OS Can Used Total RAM:%#018lx\n", totMem);

    // align the memory blocks into 4K
    totMem = 0;
    for (int i = 0; i <= memManageStruct.e820Size; i++) {
        u64 st, ed;
        if (memManageStruct.e820[i].type != 1) continue;
        st = PAGE_2M_ALIGN(memManageStruct.e820[i].address);
        ed = ((memManageStruct.e820[i].address + memManageStruct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (ed <= st) continue;
        totMem += (ed - st) >> PAGE_2M_SHIFT;
    }
    printk(WHITE, BLACK, "OS can used total 2M pages : %#018lx = %llu\n", totMem, totMem);

    // the memory managed is the memory after that of kernel program.

    // init bitmap
    totMem = memManageStruct.e820[memManageStruct.e820Size].address + memManageStruct.e820[memManageStruct.e820Size].length;
    memManageStruct.bitmap = (u64 *)PAGE_4K_ALIGN(memManageStruct.brkEd);
    // aligned to 8
    memManageStruct.bitmapLength = totMem >> PAGE_2M_SHIFT;
    memManageStruct.bitmapSize = ((memManageStruct.bitmapLength + sizeof(u64) * 8 - 1) / 8) & (~(sizeof(u64) - 1));
    // let the state of all the pages to "used"
    memset(memManageStruct.bitmap, 0xff, memManageStruct.bitmapSize);
    
    // init pages
    memManageStruct.pages = (Page *)PAGE_4K_ALIGN((u64)memManageStruct.bitmap + memManageStruct.bitmapSize);
    memManageStruct.pagesLength = totMem >> PAGE_2M_SHIFT;
    memManageStruct.pagesSize = (((totMem >> PAGE_2M_SHIFT) * sizeof(Page) + sizeof(u64) - 1) & (~(sizeof(u64) - 1)));
    memset(memManageStruct.pages, 0x00, memManageStruct.pagesSize);

    // init zones
    memManageStruct.zones = (Zone *)PAGE_4K_ALIGN((u64)memManageStruct.pages + memManageStruct.pagesSize);
    memManageStruct.zonesLength = 0;
    memManageStruct.zonesSize = (5 * sizeof(Zone) + sizeof(u64) - 1) & (~(sizeof(long) - 1));
    memset(memManageStruct.zones, 0x00, memManageStruct.zonesSize);

    for (int i = 0; i <= memManageStruct.e820Size; i++) {
        u64 st, ed; Zone *zone; Page *page; u64 *bit;
        if (memManageStruct.e820[i].type != 1) continue;
        st = PAGE_2M_ALIGN(memManageStruct.e820[i].address);
        ed = ((memManageStruct.e820[i].address + memManageStruct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (ed <= st) continue;

        zone = memManageStruct.zones + memManageStruct.zonesLength;
        memManageStruct.zonesLength++;

        zone->stAddr = st, zone->edAddr = ed;
        zone->length = ed - st;
        zone->pageUsingCnt = 0;
        zone->pageFreeCnt = (ed - st) >> PAGE_2M_SHIFT;

        zone->totPageLink = 0;

        zone->attribute = 0;
        zone->blgGMD = &memManageStruct;

        zone->pagesLength = (ed - st) >> PAGE_2M_SHIFT;
        zone->pages = memManageStruct.pages + (st >> PAGE_2M_SHIFT);

        // init the pages that belongs to this zone
        page = zone->pages;
        for (int j = 0; j < zone->pagesLength; j++, page++) {
            page->blgZone = zone;
            page->phyAddr = st + PAGE_2M_SIZE * j;
            page->attribute = 0;
            page->refCnt = 0;
            page->age = 0;
            // set the state of the page to "free"
            *(memManageStruct.bitmap + ((page->phyAddr >> PAGE_2M_SHIFT) >> 6)) ^= 1ul << ((page->phyAddr >> PAGE_2M_SHIFT) % 64);
        }
    }
    // set the pages[0] to zone[0]
    memManageStruct.pages->blgZone = memManageStruct.zones;
    memManageStruct.pages->phyAddr = 0ul;
    memManageStruct.pages->attribute = 0;
    memManageStruct.pages->refCnt = 0;
    memManageStruct.pages->age = 0;

    // reset the memory occupied by zones
    memManageStruct.zonesSize = (memManageStruct.zonesLength * sizeof(Zone) + sizeof(u64) - 1) & (~(sizeof(u64) - 1));
    
    // print the information of bitmap, pages and zones
    printk(WHITE, BLACK, "Bitmap:%#018lx, Length:%#018lx, Size:%#018lx\n", memManageStruct.bitmap, memManageStruct.bitmapLength, memManageStruct.bitmapSize);
    printk(WHITE, BLACK, "Pages:%#018lx, Length:%#018lx, Size:%#018lx\n", memManageStruct.pages, memManageStruct.pagesLength, memManageStruct.pagesSize);
    printk(WHITE, BLACK, "Zones:%#018lx, Length:%#018lx, Size:%#018lx\n", memManageStruct.zones, memManageStruct.zonesLength, memManageStruct.zonesSize);

    dmaIndex = normalIndex = 0;

    for (int i = 0; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        // print the information of this zone
        printk(GREEN, BLACK, "Zone:%d, Start:%#018lx, End:%#018lx, Length:%#018lx, Pages:%#018lx, PageLength:%#018lx\n", 
            i, zone->stAddr, zone->edAddr, zone->length, zone->pages, zone->pagesLength);
        // set unmapIndex to the index of the zone that has the start address of 1GB
        if (zone->stAddr == 0x100000000ul) unmapIndex = i;
    }

    // set the end address of memManageStruct
    memManageStruct.endOfStruct = ((u64)memManageStruct.zones + memManageStruct.zonesSize + sizeof(u64) * 32) & (~(sizeof(u64) - 1));

    // print the information of memory of kernel program
    printk(WHITE, BLACK, "Kernel Start:%#018lx, End:%#018lx, Data End:%#018lx, Break End:%#018lx\n", 
        memManageStruct.codeSt, memManageStruct.codeEd, memManageStruct.dataEd, memManageStruct.brkEd);

    Buddy_initMemory();

    u64 to = PAGE_2M_ALIGN(virtToPhy(memManageStruct.endOfStruct)) >> PAGE_2M_SHIFT;
    for (u64 i = 0; i < to; i++) 
        initPage(memManageStruct.pages + i, PAGE_PTable_Maped | PAGE_Kernel_Init | PAGE_Active | PAGE_Kernel),
        printk(YELLOW, BLACK, "Set page %d as the memory manage page\n", i);

    Buddy_initStruct();

    globalCR3 = getCR3();
    printk(WHITE, BLACK, "Global CR3:%#018lx\n", globalCR3);
    printk(WHITE, BLACK, "*Global CR3:%#018lx\n", *phyToVirt(globalCR3) & (~0xff));
    printk(WHITE, BLACK, "**Global CR3:%#018lx\n", *phyToVirt(*phyToVirt(globalCR3) & (~0xff)) & (~0xff));
    
    flushTLB();
} 

Page *allocPages(int zoneSel, int num, u64 flags) {
    int zoneSt = 0, zoneEd = 0;
    switch (zoneSel) {
        case ZONE_DMA: zoneSt = 0, zoneEd = dmaIndex; break;
        case ZONE_NORMAL: zoneSt = dmaIndex, zoneEd = normalIndex; break;
        case ZONE_UNMAPED: zoneSt = normalIndex, zoneEd = memManageStruct.zonesLength - 1; break;
        default: 
            printk(RED, BLACK, "allocPages: zoneSel error\n");
            return NULL;
    }
    for (int i = zoneSt; i <= zoneEd; i++) {
        if (memManageStruct.zones[i].pageFreeCnt < num) continue;
        Zone *zone = memManageStruct.zones + i;
        u64 st = zone->stAddr >> PAGE_2M_SHIFT, ed = zone->edAddr >> PAGE_2M_SHIFT,
            len = zone->length >> PAGE_2M_SHIFT;
        u64 tmp = 64 - st % 64;
        for (int j = st; j <= ed; j += (j % 64 ? tmp : 64)) {
            u64 *bits = memManageStruct.bitmap + (j >> 6),
                shift = j % 64;
            for (u64 k = shift; k < 64 - shift; k++) {
                if (!(((*bits >> k) | (*(bits + 1) << (64 - k))) & (num == 64 ? 0xffffffffffffffff : ((1ul << num) - 1)))) {
                    for (int l = 0; l < num; l++) {
                        Page *page = zone->pages + (j + k - 1) + l;
                        initPage(page, flags);
                    }
                    return zone->pages + j + k - 1;
                }
            }
        }
    }
    return NULL;
}