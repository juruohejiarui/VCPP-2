#include "DMAS.h"
#include "pgtable.h"
#include "../includes/log.h"

void DMAS_init() {
    printk(WHITE, BLACK, "DMAS_init\n");
    // calculate the page table size of DMAS
    u64 pudEntryCnt = Page_1GUpAlign(gloMemManageStruct.totMemSize) >> Page_1GShift,
        pudCnt = (pudEntryCnt + 511) / 512;
    u64 ptSize = pudCnt * Page_4KSize;
    printk(WHITE, BLACK, "pudEntryCnt = %ld, pudCnt = %ld, ptSize = %ld\n", pudEntryCnt, pudCnt, ptSize);
    // write the page table behind the kernel program
    u64 *ptStart = (u64 *)Page_4KUpAlign(gloMemManageStruct.edAddrOfStruct);
    for (int i = 0; i < pudEntryCnt; i++) {
        *(ptStart + i) = (u64)Page_4KUpAlign(DMAS_physAddrStart + i * Page_1GSize) | 0x87;
        if (i % 512 == 0) printk(ORANGE, BLACK, "Table Base Address : %#018lx\n", ptStart + i);
        printk(WHITE, BLACK, "ptStart[%d] = %#018lx\n", i, *(ptStart + i));
    }
    gloMemManageStruct.edAddrOfStruct = Page_4KUpAlign((u64)ptStart + ptSize);
    // set the PGD entry of DMAS
    u64 *pgd = (u64 *)(getCR3() + Init_virtAddrStart);
    // printk(RED, BLACK, "pgd = %#018lx, pgd[256] = %#018lx\n", pgd, pgd[256]);
    for (int i = 256 + 16; i < 256 + 16 + pudCnt; i++) {
        pgd[i] = ((u64)ptStart + (i - 256 - 16) * Page_4KSize) - Init_virtAddrStart;
        pgd[i] |= 0x07;
        printk(WHITE, BLACK, "%#018lx -> %#018lx\n", i * 512ll << 30, pgd[i]);
    }
    flushTLB();
}