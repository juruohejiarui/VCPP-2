#include "desc.h"
#include "pgtable.h"
#include "../includes/hardware.h"
#include "../includes/log.h"
#include "DMAS.h"

struct GlobalMemManageStruct memManageStruct = {{0}, 0};

// initialize the zones, pages and bitmaps
static void initArray() {
    printk(RED, BLACK, "initArray()\n");
    int kernelZoneId = -1;
    u64 totPage = 0;
    memManageStruct.zonesLength = 0;
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
        if (e820->type != 1) continue;
        u64 st = Page_4KUpAlign(e820->addr), ed = Page_4KDownAlign(e820->addr + e820->size);
        if (st >= ed) continue;
        if (0x100000 >= st && ed > memManageStruct.edOfStruct - Init_virtAddrStart)
            kernelZoneId = i;
        memManageStruct.zonesLength++;
        totPage = (ed - st) >> Page_4KShift;
    }
    printk(WHITE, BLACK, "kernelZoneId = %d\n", kernelZoneId);
    // search for a place for building zones
    u64 reqSize = upAlignTo(sizeof(Zone) * memManageStruct.zonesLength, sizeof(u64));
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        E820 *e820 = memManageStruct.e820 + i;
        if (e820->type != 1) continue;
        u64 availdSt = (kernelZoneId == i ? (memManageStruct.edOfStruct - Init_virtAddrStart) : e820->addr),
            ed = Page_4KDownAlign(e820->addr + e820->size);
        if (availdSt + reqSize >= ed) continue;
        // build the system in this zone
        memset(DMAS_phys2Virt(availdSt), 0, reqSize);
        printk(RED, BLACK, "Set the zone array on %#018lx\n", DMAS_phys2Virt(availdSt));
        memManageStruct.zones = DMAS_phys2Virt(availdSt);
        for (int j = 0, id = 0; j <= memManageStruct.e820Length; j++) {
            E820 *e820 = memManageStruct.e820 + j;
            if (e820->type != 1) continue;
            Zone *zone = memManageStruct.zones + id;
            zone->phyAddrSt = Page_4KUpAlign(e820->addr);
            zone->phyAddrEd = Page_4KDownAlign(e820->addr + e820->size);
            zone->attribute = zone->phyAddrSt;
            if (kernelZoneId == j) zone->attribute = memManageStruct.edOfStruct - Init_virtAddrStart;
            if (j == i) zone->attribute += reqSize;
            printk(RED, BLACK, "zone[%d]: phyAddr: [%#018lx, %#018lx], attribute = %#018lx\n", 
                i, zone->phyAddrSt, zone->phyAddrEd, zone->attribute);
            id++;
        }
        goto FindAvailSpace;
    }
    printk(RED, BLACK, "Fail to build a zone array\n");
    return ;
    FindAvailSpace:
    reqSize = upAlignTo(sizeof(Page) * totPage, sizeof(u64));
}

void Init_memManage() {
    EFI_E820MemoryDescriptor *p = (EFI_E820MemoryDescriptor *)bootParamInfo->E820Info.entry;
    u64 totMem = 0;
    printk(WHITE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    for (int i = 0; i < bootParamInfo->E820Info.entryCount; i++) {
        printk(GREEN, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        if (p->type == 1) totMem += p->length;
        memManageStruct.e820[i].addr = p->address;
        memManageStruct.e820[i].size = p->length;
        memManageStruct.e820[i].type = p->type;
        memManageStruct.e820Length = i;
        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1) break;
    }
    printk(WHITE, BLACK, "Total Memory Size : %#018lx\n", totMem);
    memManageStruct.totMemSize = totMem;

    // get the total 4K pages
    totMem = 0;
    for (int i = 0; i <= memManageStruct.e820Length; i++) {
        u64 st, ed;
        if (memManageStruct.e820[i].type != 1) continue;
        st = Page_4KUpAlign(memManageStruct.e820[i].addr);
        ed = Page_4KDownAlign(memManageStruct.e820[i].addr + memManageStruct.e820[i].size);
        if (ed <= st) continue;
        totMem = (ed - st) >> Page_4KShift;
    }
    printk(WHITE, BLACK, "Total 4K pages: %#018lx = %ld\n", totMem, totMem);

    memManageStruct.edOfStruct = Page_4KUpAlign((u64)&_end);
    printk(WHITE, BLACK, "edAddrOfStruct = %#018lx\n", memManageStruct.edOfStruct);

    DMAS_init();
    initArray();
}