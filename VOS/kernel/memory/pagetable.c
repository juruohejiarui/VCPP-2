#include "memoryinner.h"
#include "../includes/task.h"
#include "../includes/printk.h"

#define pageTableSize 512

u64 *initSwpAddr, *swpAddr, *swpPDGAddr;

// This function can only used when getCR3() == 0x101000
u64 *getPDG(u64 virtAddr) {
    // get phy address of pdpte
    return (u64 *) (
            // get address of of pdg
            *(phyToVirt(
                // get address of pdpte
                *(  phyToVirt(
                        (u64)getCR3())
                         + ((virtAddr >> 39) & 0x1ff)
                ) & ~0xfff)
                + ((virtAddr >> 30) & 0x1ff)
            ) & ~0xfff)
         + ((virtAddr >> 21) & 0x1ff);
}

void PageTable_init() {
    // get a 2M page for swap space
    swpAddr = (u64 *)PAGE_2M_ALIGN(memManageStruct.endOfStruct);
    memManageStruct.endOfStruct = (u64)swpAddr + PAGE_2M_SIZE;
    swpPDGAddr = getPDG((u64)swpAddr);
    printk(RED, BLACK, "swpAddr = %#018lx, swpPDGAddr = %#018lx\n", swpAddr, swpPDGAddr);
}

void cpyPdgte(u64 *stAddr, u64 *swpAddr, u64 *trgVirtAddr) {
    memcpy(stAddr, swpAddr, sizeof(u64) * 512);
}

#define pageTableSize 512
// initialize the page table on the physics address PHY_ADDR and set its virtAddr to TARGET_VIRT_ADDR,
// this action will copy the kernel page table
// PS: ensure that the phyAddr is aligned to 2M
void PageTable_initPageTable(u64 phyAddr, u64 *trgVirtAddr) {
    u64 *oldCR3 = getCR3();
    setCR3(originalCR3);
    u64 oldAddr = *swpPDGAddr;
    // use swap page to do this task
    *swpPDGAddr = phyAddr | 0x87;
    // struct overview:
    // page table size
    // [pml4e + virt address of each pdpte]
    // [pdpte + virt address of each pgd of this pdpte] * n
    // [pdg] * n
    // copy the original
    // copy pml4e
    u64 bsOffset = 2 * pageTableSize;
    memcpy(phyToVirt((u64 *)getCR3()), swpAddr, sizeof(u64) * 512);
    // copy pdgte
    // set the virtual address of the page table
    u64 validCnt = 0, validCnt2 = 0;
    for (int i = 256; i < pageTableSize; i++) {
        if (*phyToVirt(getCR3() + i) == 0) continue;
        *(swpAddr + i + pageTableSize) = (u64)(trgVirtAddr + bsOffset + validCnt * 2 * pageTableSize);
        memcpy(phyToVirt(*phyToVirt(getCR3() + i) & ~0xfff), swpAddr + bsOffset + validCnt * 2 * pageTableSize, 512 * sizeof(u64));
        validCnt++;
    }
    bsOffset += validCnt * 2 * pageTableSize;
    validCnt = 0;
    // search each pdpte
    for (int i = 256; i < pageTableSize; i++) {
        if (*phyToVirt(getCR3() + i) == 0) continue;
        u64 *pdpteAddr = phyToVirt(*phyToVirt(getCR3() + i) & ~0xfff);
        for (int j = 0; j < pageTableSize; j++) {
            if (*(pdpteAddr + j) == 0) continue;
            // set the virtual address of this pdg
            *(swpAddr + 2 * pageTableSize + (validCnt * 2 + 1) * pageTableSize + j) = (u64)(trgVirtAddr + bsOffset + validCnt2 * 2 * pageTableSize);
            memcpy(phyToVirt(*(pdpteAddr + j) & ~0xfff), swpAddr + bsOffset + validCnt2 * 2 * pageTableSize, 512 * sizeof(u64));
            // validCnt2++;
        }
        validCnt++;
    }
    for (int i = 0; i < PAGE_2M_SIZE / sizeof(u64); i++) if (*(swpAddr + i) != 0)
        printk(BLACK, WHITE, "%#018lx: %016lx\n", swpAddr + i, *(swpAddr + i));
    *swpPDGAddr = oldAddr;
    setCR3((u64)oldCR3);
    for (int i = 0; i < PAGE_2M_SIZE / sizeof(u64); i++) if (*(swpAddr + i) != 0)
        printk(BLACK, WHITE, "-%#018lx: %016lx\n", swpAddr + i, *(swpAddr + i));
}

void PageTable_map(u64 phyAddr, u64 *virtAddrSt, u64 *virtAddrEd) {
    Pml4t *pgTable = current->memStruct->pgd;
}

void PageTable_unmap(u64 *virtAddrSt, u64 *virtAddrEd) {

}

void PageTable_debug(u64 *targetVirtAddr) {

}