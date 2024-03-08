#include "../includes/memory.h"
#include "../includes/UEFI.h"
#include "../includes/printk.h"

struct GlobalMemoryDescriptor memManageStruct = {{0}, 0};


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
        st = PAGE_4K_ALIGN(memManageStruct.e820[i].address);
        ed = ((memManageStruct.e820[i].address + memManageStruct.e820[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        if (ed <= st) continue;
        totMem += (ed - st) >> PAGE_4K_SHIFT;
    }
    printk(WHITE, BLACK, "OS can used total 4K pages : %#018lx = %llu\n", totMem, totMem);

    // the memory managed is the memory after that of kernel program.

    // init bitmap
    totMem = memManageStruct.e820[memManageStruct.e820Size].address + memManageStruct.e820[memManageStruct.e820Size].length;
    memManageStruct.bitmap = (u64 *)PAGE_4K_ALIGN(memManageStruct.brkEd);
    // aligned to 8
    memManageStruct.bitmapLength = ((totMem >> PAGE_4K_SHIFT) + sizeof(u64) - 1) / sizeof(u64);
    memManageStruct.bitmapSize = memManageStruct.bitmapLength * sizeof(u64);
    // let the state of all the pages be "free"
    memset(memManageStruct.bitmap, 0xff, memManageStruct.bitmapSize);
    
    // init pages
    memManageStruct.pages = (Page *)PAGE_4K_ALIGN((u64)memManageStruct.bitmap + memManageStruct.bitmapSize);
    memManageStruct.pagesLength = totMem >> PAGE_4K_SHIFT;
    memManageStruct.pagesSize = (((totMem >> PAGE_4K_SHIFT) * sizeof(Page) + sizeof(u64) - 1) & (~(sizeof(u64) - 1)));
    memset(memManageStruct.pages, 0x00, memManageStruct.pagesSize);

    // init zones
    memManageStruct.zones = (Zone *)PAGE_4K_ALIGN((u64)memManageStruct.pages + memManageStruct.pagesSize);
    memManageStruct.zonesLength = 0;
    memManageStruct.zonesSize = (5 * sizeof(Zone) + sizeof(u64) - 1) & (~(sizeof(long) - 1));
    memset(memManageStruct.zones, 0x00, memManageStruct.zonesSize);

    for (int i = 0; i <= memManageStruct.e820Size; i++) {
        u64 st, ed; Zone *zone; Page *page; u64 *bit;
        if (memManageStruct.e820[i].type != 1) continue;
        st = PAGE_4K_ALIGN(memManageStruct.e820[i].address);
        ed = ((memManageStruct.e820[i].address + memManageStruct.e820[i].length) >> PAGE_4K_SHIFT) << PAGE_4K_SHIFT;
        if (ed <= st) continue;

        zone = memManageStruct.zones + memManageStruct.zonesLength;
        memManageStruct.zonesLength++;

        zone->stAddr = st, zone->edAddr = ed;
        zone->length = ed - st;
        zone->pageUsingCnt = 0;
        zone->pageFreeCnt = (ed - st) >> PAGE_4K_SHIFT;

        zone->totPageLink = 0;

        zone->attribute = 0;
        zone->blgGMD = &memManageStruct;

        zone->pagesLength = (ed - st) >> PAGE_4K_SHIFT;
        zone->pages = memManageStruct.pages + (st >> PAGE_4K_SHIFT);

        // init the pages that belongs to this zone
        page = zone->pages;
        for (int j = 0; j < zone->pagesLength; j++, page++) {
            page->blgZone = zone;
            page->phyAddr = st + PAGE_4K_SIZE * j;
            page->attribute = 0;
            page->refCnt = 0;
            page->age = 0;

            *(memManageStruct.bitmap + ((page->phyAddr >> PAGE_4K_SHIFT) >> 6)) ^= 1ul << ((page->phyAddr >> PAGE_4K_SHIFT) % 64);
        }
    }
} 