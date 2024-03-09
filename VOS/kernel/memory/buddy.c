#include "buddy.h"
#include "../includes/memory.h"
#include "../includes/printk.h"

struct BuddyStruct buddyManageStruct;

void initBuddy() {
    for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
        buddyManageStruct.head[i] = NULL;
        buddyManageStruct.freeHead[i] = NULL;
    }
    // create (memManageStruct.pagesLength) new list elements for buddy system
    buddyManageStruct.freeListEle = (BuddyList *)memManageStruct.endOfStruct;
    memManageStruct.endOfStruct += sizeof(BuddyList) * memManageStruct.pagesLength;
    BuddyList *listEle = buddyManageStruct.freeListEle;
    for (int i = 0; i < memManageStruct.pagesLength - 1; i++) {
        listEle->next = listEle + 1;
        listEle = listEle->next;
    }
    listEle->next = NULL;

    // apply the list elements to freehead
    for (int zId = 0; zId < memManageStruct.zonesLength; zId++) {
        u64 resLen = memManageStruct.zones[zId].pagesLength, pos = 0;
        for (int i = BUDDY_MAX_ORDER - 1; i >= 0; i--) {
            if ((1ul << i) > resLen) {
                buddyManageStruct.freeHead[i] = NULL;
                continue;
            }
            printk(RED, BLACK, "Layout %d: ", i);
            BuddyList *p = buddyManageStruct.freeListEle;
            buddyManageStruct.freeHead[i] = p;
            p->pageAddr = &memManageStruct.zones[zId].pages[pos];
            printk(RED, BLACK, "%lld %#018lx\t", pos, p->pageAddr);
            resLen -= (1ul << i), pos += (1ul << i);
            buddyManageStruct.freeListEle = p->next;
            while (resLen >= (1ul << i)) {
                p->next = buddyManageStruct.freeListEle;
                buddyManageStruct.freeListEle->prev = p;
                buddyManageStruct.freeListEle = buddyManageStruct.freeListEle->next;
                buddyManageStruct.freeListEle->prev = NULL;
                p = p->next;
                p->next = NULL;
                p->pageAddr = &memManageStruct.zones[zId].pages[pos];
                printk(RED, BLACK, "%lld %#018lx\t", pos, p->pageAddr);
                resLen -= (1ul << i), pos += (1ul <<  i);
            }
            putchar(RED, BLACK, '\n');
        }
    }
    
}