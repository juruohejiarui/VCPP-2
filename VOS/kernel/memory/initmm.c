// this file defines the initialization process of memory management and the APIs of basic management system
#include "desc.h"
#include "pgtable.h"
#include "../includes/hardware.h"
#include "../includes/log.h"
#include "DMAS.h"
#include "buddy.h"
#include "SLAB.h"

struct GlobalMemManageStruct memManageStruct = {{0}, 0};

// initialize the zones, pages and bitmaps
static void _initArray() {
	#ifdef DEBUG_MM
    printk(RED, BLACK, "->initArray()\n");
	#endif
    int kernelZoneId = -1;
    u64 totPage = 0;
    memManageStruct.zonesLength = 0;
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
        if (e820->type != 1) continue;
        u64 st = Page_4KUpAlign(e820->addr), ed = Page_4KDownAlign(e820->addr + e820->size);
        if (st >= ed) continue;
        if (0x100000 >= st && ed > memManageStruct.edOfStruct - Init_virtAddrStart && memManageStruct.zonesLength > 0)
            kernelZoneId = i;
        memManageStruct.zonesLength++;
    }
    totPage = (memManageStruct.e820[memManageStruct.e820Length].addr + memManageStruct.e820[memManageStruct.e820Length].size) >> Page_4KShift;
    // search for a place for building zones
    u64 reqSize = upAlignTo(sizeof(Zone) * memManageStruct.zonesLength, sizeof(u64));
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
        if (e820->type != 1) continue;
        u64 availdSt = max((kernelZoneId == i ? (memManageStruct.edOfStruct - Init_virtAddrStart) : e820->addr), (u64)availVirtAddrSt - Init_virtAddrStart),
            ed = Page_4KDownAlign(e820->addr + e820->size);
        if (availdSt + reqSize >= ed) continue;
        // build the system in this zone
        memset(DMAS_phys2Virt(availdSt), 0, reqSize);
		#ifdef DEBUG_MM
        printk(ORANGE, BLACK, "Set the zone array on %#018lx\n", DMAS_phys2Virt(availdSt));
		#endif
        memManageStruct.zones = DMAS_phys2Virt(availdSt);
        for (int j = 0, id = 0; j <= memManageStruct.e820Length; j++) {
            E820 *e820 = memManageStruct.e820 + j;
            if (e820->type != 1) continue;
            Zone *zone = memManageStruct.zones + id;
            zone->phyAddrSt = Page_4KUpAlign(e820->addr);
            zone->phyAddrEd = Page_4KDownAlign(e820->addr + e820->size);
            zone->attribute = zone->phyAddrSt;
            if (j < kernelZoneId) zone->attribute = zone->phyAddrEd;
            if (zone->phyAddrSt <= (u64)availVirtAddrSt - Init_virtAddrStart && zone->phyAddrEd > (u64)availVirtAddrSt - Init_virtAddrStart)
                zone->attribute = Page_4KUpAlign((u64)availVirtAddrSt - Init_virtAddrStart);
            if (j == i) zone->attribute += reqSize;
			#ifdef DEBUG_MM
            printk(WHITE, BLACK, "zone[%d]: phyAddr: [%#018lx, %#018lx], attribute = %#018lx\n", 
                id, zone->phyAddrSt, zone->phyAddrEd, zone->attribute);
			#endif
            id++;
        }
        goto SuccBuildZones;
    }
    printk(RED, BLACK, "Fail to build a zone array\n");
    return ;
    SuccBuildZones:
    reqSize = upAlignTo(sizeof(Page) * totPage, sizeof(u64));
	#ifdef DEBUG_MM
    printk(RED, BLACK, "%ld->%d\n", sizeof(Page) * totPage, reqSize);
	#endif
    memManageStruct.pagesLength = totPage;
    for (int i = 1; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        if (zone->attribute + reqSize >= zone->phyAddrEd) continue;
        memset(DMAS_phys2Virt(zone->attribute), 0, reqSize);
		#ifdef DEBUG_ALLOC
        printk(ORANGE, BLACK, "Set the page array on %#018lx, size = %#018lx\n", DMAS_phys2Virt(zone->attribute), reqSize);
		#endif
        memManageStruct.pages = DMAS_phys2Virt(zone->attribute);
        zone->attribute = upAlignTo(zone->attribute + reqSize, sizeof(u64));
        for (u64 j = 0; j < totPage; j++)
            memManageStruct.pages[j].phyAddr = j * Page_4KSize;
        goto SuccBuildPages;
    }
    printk(RED, BLACK, "Fail to build a page array\n");
    return ;
    SuccBuildPages:
    // set the property "blgZone" of the pages
    for (int i = 0; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        zone->pages = memManageStruct.pages + zone->phyAddrSt / Page_4KSize;
        zone->pagesLength = (zone->phyAddrEd - zone->phyAddrSt) / Page_4KSize;
        zone->usingCnt = Page_4KUpAlign(zone->attribute) / Page_4KSize - zone->phyAddrSt / Page_4KSize;
        zone->freeCnt = zone->pagesLength - zone->usingCnt;
        for (u64 j = 0; j < zone->usingCnt; j++)
            zone->pages[j].attr = Page_Flag_Kernel | Page_Flag_KernelInit;
        for (int i = 0; i < zone->pagesLength; i++) zone->pages[i].blgZone = zone;
    }
    return ;
}

void MM_init() {
    printk(RED, BLACK, "MM_init()\n");
    EFI_E820MemoryDescriptor *p = (EFI_E820MemoryDescriptor *)HW_UEFI_bootParamInfo->E820Info.entry;
    u64 totMem = 0;
    // printk(WHITE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    for (int i = 0; i < HW_UEFI_bootParamInfo->E820Info.entryCount; i++) {
        // printk(GREEN, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        totMem += p->length;
        memManageStruct.e820[i].addr = p->address;
        memManageStruct.e820[i].size = p->length;
        memManageStruct.e820[i].type = p->type;
        memManageStruct.e820Length = i;
        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1) break;
    }
    memManageStruct.totMemSize = totMem;

    // get the total 4K pages
    totMem = 0;
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        u64 st, ed;
        if (memManageStruct.e820[i].type != 1) continue;
        st = Page_4KUpAlign(memManageStruct.e820[i].addr);
        ed = Page_4KDownAlign(memManageStruct.e820[i].addr + memManageStruct.e820[i].size);
        if (ed <= st) continue;
        totMem += (ed - st) >> Page_4KShift;
    }
	#ifdef DEBUG_MM
    printk(WHITE, BLACK, "Total 4K pages: %#018lx = %ld\t", totMem, totMem);
	#endif

    memManageStruct.edOfStruct = Page_4KUpAlign((u64)&_end);
    
    flushTLB();

    DMAS_init();
    _initArray();
    MM_Buddy_init();
    MM_PageTable_init();

	#ifdef DEBUG_MM
    printk(WHITE, BLACK, "totMemSize = %#018lx Byte = %ld MB\n", memManageStruct.totMemSize, memManageStruct.totMemSize >> 20);
	#endif

    MM_Slab_init();
}

inline void MM_Bs_setPageAttr(Page *page, u64 attr) { page->attr = attr; }
inline u64 MM_Bs_getPageAttr(Page *page) { return page->attr; }

Page *MM_Bs_alloc(u64 num, u64 attr) {
    for (int i = 1; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        if (zone->freeCnt < num) continue;
        // check if there are enough continuous pages
        Page *stPage = zone->pages + zone->usingCnt;
        zone->usingCnt += num;
        u64 bitPos = stPage - memManageStruct.pages;
        // set the attribute of the pages
        for (int j = 0; j < num; j++) 
            MM_Bs_setPageAttr(stPage + j, attr);
        return stPage;
    }
    return NULL;
}