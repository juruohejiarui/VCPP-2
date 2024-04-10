#include "desc.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

struct GlobalMemManageStruct gloMemManageStruct = {{0}, 0};

void Init_DMMA() {

}

void Init_memManage() {
    EFI_E820MemoryDescriptor *p = (EFI_E820MemoryDescriptor *)bootParamInfo->E820Info.entry;
    u64 totMem = 0;
    printk(WHITE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    for (int i = 0; i < bootParamInfo->E820Info.entryCount; i++) {
        printk(GREEN, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        if (p->type == 1) totMem += p->length;
        gloMemManageStruct.e820[i].addr = p->address;
        gloMemManageStruct.e820[i].size = p->length;
        gloMemManageStruct.e820[i].type = p->type;
        gloMemManageStruct.e820Length = i;
        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1) break;
    }
    printk(WHITE, BLACK, "Total Memory Size : %#018lx\n", totMem);

    // get the total 4K pages
    totMem = 0;
    for (int i = 0; i <= gloMemManageStruct.e820Length; i++) {
        u64 st, ed;
        if (gloMemManageStruct.e820[i].type != 1) continue;
        st = Page_4KUpAlign(gloMemManageStruct.e820[i].addr);
        ed = Page_4KDownAlign(gloMemManageStruct.e820[i].addr + gloMemManageStruct.e820[i].size);
        if (ed <= st) continue;
        totMem = (ed - st) >> Page_4KShift;
    }
    printk(WHITE, BLACK, "Total 4K pages: %#018lx = %ld\n", totMem, totMem);

    gloMemManageStruct.edAddrOfStruct = Page_4KUpAlign((u64)&_end);
}