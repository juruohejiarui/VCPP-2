#include "desc.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

struct GlobalMemManageStruct gloMemManageStruct = {{0}, 0};

void Init_memManage() {
    EFI_E820MemoryDescriptor *p = (EFI_E820MemoryDescriptor *)bootParamInfo->E820Info.entry;
    u64 totMem = 0;
    printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    for (int i = 0; i < 32; i++) {
        printk(GREEN, BLACK, "UEFI=>KERN---Address:%#018lx\tLength:%#018lx\tType:%#010x\n", p->address, p->length, p->type);
        if (p->type == 1) totMem += p->length;
        gloMemManageStruct.e820[i].addr = p->address;
        gloMemManageStruct.e820[i].size = p->length;
        gloMemManageStruct.e820[i].type = p->type;
        gloMemManageStruct.e820Length = i;
        p++;
        if (p->type == 4 || p->length == 0 || p->type < 1) break;
    }
    print(WHITE, BLACK, "Total Memory Size : %#018lx\n", totMem);
}