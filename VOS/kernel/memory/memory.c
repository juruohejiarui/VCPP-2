#include "../includes/memory.h"
#include "../includes/UEFI.h"
#include "../includes/printk.h"

struct GlobalMemoryDescriptor memManageStruct = {{0}, 0};


void initMemory() {
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
} 