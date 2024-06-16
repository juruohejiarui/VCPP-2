#include "DMAS.h"
#include "pgtable.h"
#include "../includes/log.h"

void DMAS_init() {
	#ifdef DEBUG_MM
    printk(WHITE, BLACK, "DMAS_init\n");
	#endif
    // calculate the page table size of DMAS
    u64 phyAddrEnd = memManageStruct.e820[memManageStruct.e820Length].addr + memManageStruct.e820[memManageStruct.e820Length].size;
    u64 pudEntryCnt = max(8, Page_1GUpAlign(phyAddrEnd) >> Page_1GShift),
        pudCnt = (pudEntryCnt + 511) / 512;
    u64 ptSize = pudCnt * Page_4KSize;
	#ifdef DEBUG_MM
    printk(WHITE, BLACK, "pudEntryCnt = %ld, pudCnt = %ld, ptSize = %ld\n", pudEntryCnt, pudCnt, ptSize);
	#endif
    // write the page table behind the kernel program
    u64 *ptStart = (u64 *)Page_4KUpAlign(memManageStruct.edOfStruct);
    for (int i = 0; i < pudEntryCnt; i++) {
        *(ptStart + i) = (u64)Page_4KUpAlign(DMAS_physAddrStart + i * Page_1GSize) | 0x87;
		#ifdef DEBUG_MM
        if (i % 512 == 0) printk(ORANGE, BLACK, "Table Base Address : %#018lx\n", ptStart + i);
		#endif
    }
    memManageStruct.edOfStruct = Page_4KUpAlign((u64)ptStart + ptSize);
    // set the PGD entry of DMAS
    u64 *pgd = (u64 *)(getCR3() + Init_virtAddrStart);
    for (int i = 256 + 16; i < 256 + 16 + pudCnt; i++) {
        pgd[i] = ((u64)ptStart + (i - 256 - 16) * Page_4KSize) - Init_virtAddrStart;
        pgd[i] |= 0x07;
		#ifdef DEBUG_MM
        printk(WHITE, BLACK, "%#018lx -> %#018lx\n", i * 512ll << 30, pgd[i]);
		#endif
    }
    flushTLB();
}