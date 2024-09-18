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
    int kernelZoneId = -1, lstType1 = 0;
    memManageStruct.zonesLength = 0;
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
		#ifdef DEBUG_MM
		printk(YELLOW, BLACK, "e820[%02d]: addr:%#018lx len:%#018lx type:%#018lx\n", i, e820->addr, e820->size, e820->type);
		#endif
        if (e820->type != 1) continue;
        u64 st = Page_4KUpAlign(e820->addr), ed = Page_4KDownAlign(e820->addr + e820->size);
        if (st >= ed) continue;
        if (0x100000 >= st && ed > (u64)availVirtAddrSt - Init_virtAddrStart)
            kernelZoneId = i;
        memManageStruct.zonesLength++;
		lstType1 = i;
    }
	printk(WHITE, BLACK, "kernel Zone:%d availVirtAddtSt:%#018lx\n", kernelZoneId, availVirtAddrSt);

	memManageStruct.totMemSize = memManageStruct.e820[lstType1].addr + memManageStruct.e820[lstType1].size;
    // search for a place for building zones
    u64 reqSize = upAlignTo(sizeof(Zone) * memManageStruct.zonesLength, Page_4KSize);
    for (int i = kernelZoneId; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
        if (e820->type != 1) continue;
        u64 availdSt = Page_4KUpAlign(i == kernelZoneId ? (u64)availVirtAddrSt - Init_virtAddrStart : e820->addr),
            ed = Page_4KDownAlign(e820->addr + e820->size);
        if (availdSt + reqSize >= ed) continue;
        // build the system in this zone
        memset(DMAS_phys2Virt(availdSt), 0, reqSize);
		#ifdef DEBUG_MM
        printk(ORANGE, BLACK, "Set the zone array on zone %d\n", i);
		#endif
        memManageStruct.zones = DMAS_phys2Virt(availdSt);
        for (int j = 0, id = 0; j <= memManageStruct.e820Length; j++) {
            E820 *e820 = memManageStruct.e820 + j;
            if (e820->type != 1) continue;
            Zone *zone = memManageStruct.zones + id;
            zone->phyAddrSt = Page_4KUpAlign(e820->addr);
            zone->phyAddrEd = Page_4KDownAlign(e820->addr + e820->size);
            zone->attribute = zone->phyAddrSt;
			// for the zone before kernel zone, which is boot zone, should not be used.
            if (j < kernelZoneId) zone->attribute = zone->phyAddrEd;
			// for the zone that includes the kernel, should ignores the pages occupied by the kernel
            else if (j == kernelZoneId) zone->attribute = Page_4KUpAlign((u64)availVirtAddrSt - Init_virtAddrStart);
            if (j == i) zone->attribute += reqSize;

			#ifdef DEBUG_MM
            printk(WHITE, BLACK, "zone[%d]: phyAddr: [%#018lx, %#018lx], attribute = %#018lx remain:%ld\n", 
                id, zone->phyAddrSt, zone->phyAddrEd, zone->attribute, zone->phyAddrEd - zone->attribute);
			#endif
            id++;
        }
		printk(WHITE, BLACK, "zones:%#018lx\n", memManageStruct.zones);
        goto SuccBuildZones;
    }
    printk(RED, BLACK, "Fail to build a zone array\n");
	while (1) IO_hlt();
    return ;
    SuccBuildZones:
	#ifdef DEBUG_MM
    printk(RED, BLACK, "%ld->%d\n", sizeof(Page) * totPage, reqSize);
	#endif
    // allocate page array for each zones
	for (int i = 0; i < memManageStruct.zonesLength; i++) {
		Zone *zone = memManageStruct.zones + i;
		if (zone->attribute == zone->phyAddrEd) { zone->pages = NULL; continue; }
		reqSize = (zone->phyAddrEd - zone->phyAddrSt) / Page_4KSize * sizeof(Page);
		if (zone->attribute + reqSize >= zone->phyAddrEd) {
			printk(RED, BLACK, "zone[%02d]: failed to create page array\n", i);
			while (1) IO_hlt();
		}
		zone->pages = DMAS_phys2Virt(zone->attribute);
		zone->pagesLength = (zone->phyAddrEd - zone->phyAddrSt) / Page_4KSize;
		zone->attribute = Page_4KUpAlign(zone->attribute + reqSize);
		zone->freeCnt = (zone->phyAddrEd - zone->attribute) / Page_4KSize;
		zone->usingCnt = (zone->attribute - zone->phyAddrSt) / Page_4KSize;
		
		#ifdef DEBUG_MM
		printk(WHITE, BLACK, "zone[%02d](%#018lx): pages:%#018lx len:%ld reqSize:%#018lx attribute:%#018lx ed:%#018lx: usingCnt:%ld\n", 
				i, zone, zone->pages, zone->pagesLength, reqSize, zone->attribute, zone->phyAddrEd, zone->usingCnt);
		#endif

		for (u64 j = 0; j < zone->pagesLength; j++) 
			zone->pages[j].phyAddr = zone->phyAddrSt + (j << Page_4KShift),
			zone->pages[j].attr = Page_Flag_Kernel | Page_Flag_KernelInit;
	}
}

void MM_init() {
    // printk(RED, BLACK, "MM_init()\n");
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
    for (int i = 0; i < memManageStruct.zonesLength; i++) {
        Zone *zone = memManageStruct.zones + i;
        if (zone->freeCnt < num) continue;
        // check if there are enough continuous pages
        Page *stPage = zone->pages + zone->usingCnt;
        zone->usingCnt += num;
        // set the attribute of the pages
        for (int j = 0; j < num; j++) 
            MM_Bs_setPageAttr(stPage + j, attr);
        return stPage;
    }
    return NULL;
}