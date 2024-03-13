#include "../includes/memory.h"
#include "../includes/printk.h"

struct BuddyStruct buddyStruct;

#define leftPosId(x) ((x) << 1)
#define rightPosId(x) ((x) << 1 | 1)

static inline Page *getParter(Page *page, int ord) {
    return (page->posId & 1 ? page - (1ul << ord) : page + (1ul << ord));
}

static inline Page *getFather(Page *page, int ord) {
    return (page->posId & 1 ? page - (1ul << ord) : page);
}

void revBit(Page *page, int ord) {
    if (page->posId == 1) return ;
    u64 bitPos = (page->posId & 1 ? getParter(page, ord) : page) - memManageStruct.pages;
    buddyStruct.bitmap[ord][bitPos / 64] ^= (1ul << (bitPos % 64));
}
int getBit(Page *page, int ord) {
    if (page->posId == 1) return 1;
    u64 bitPos = (page->posId & 1 ? getParter(page, ord) : page) - memManageStruct.pages;
    return (buddyStruct.bitmap[ord][bitPos / 64] >> (bitPos % 64)) & 1;
}

// this action will not the bit of the page
void delFreePage(Page *page) {
    int ord = *pageOrder(page);
    if (page->attribute & PAGE_HeadPage) {
        page->attribute ^= PAGE_HeadPage;
        // delete this page from the free page list
        List_del((List *)page);
        page->listEle.next = page->listEle.prev = NULL;
        if (!List_isEmpty((List *)buddyStruct.freePageList[ord]))
            ((Page *)buddyStruct.freePageList[ord]->listEle.next)->attribute |= PAGE_HeadPage;
    } else {
        List_del((List *)page);
        page->listEle.next = page->listEle.prev = NULL;
    }
}

// Add a page into freeList[order] and try to merge the connected pages into a bigger block. 
// The posId of this page should be set before calling this function.
void addPage2FreeList(Page *page, int order) {
    for (int ord = order; ord <= BUDDY_MAX_ORDER; ord++) {
        // is the right page
        revBit(page, ord);
        // if this page is in the highest order, then should be inserted.
        if (getBit(page, ord) || page->posId == 1) {
            // printk(ORANGE, BLACK, "add page %p to order %d\n", page, ord);
            if (!List_isEmpty((List *)buddyStruct.freePageList[ord]))
                ((Page *)buddyStruct.freePageList[ord]->listEle.next)->attribute ^= PAGE_HeadPage;
            page->attribute |= PAGE_HeadPage;
            List_addBehind((List *)buddyStruct.freePageList[ord], (List *)page);
            *pageOrder(page) = ord;
            break;
        } else {
            delFreePage(getParter(page, ord));
            Page *father = getFather(page, ord);
            *pageOrder(father) = ord + 1;
            father->posId = page->posId >> 1;
            page = father;
        } 
    }
}

void Buddy_initMemory() {
    u64 endOfBitmap = PAGE_4K_ALIGN(memManageStruct.endOfStruct);
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        buddyStruct.bitmap[i] = (u64 *)endOfBitmap;
        memset(buddyStruct.bitmap[i], 0x00, memManageStruct.bitmapSize);
        endOfBitmap = PAGE_4K_ALIGN(endOfBitmap + memManageStruct.bitmapSize);
    }
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        buddyStruct.freePageList[i] = (Page *)endOfBitmap;
        List_init((List *)buddyStruct.freePageList[i]);
        endOfBitmap += sizeof(Page);
    }
    memManageStruct.endOfStruct = endOfBitmap;
}
void Buddy_initStruct() {
    for (int zId = 0; zId < memManageStruct.zonesLength; zId++) {
        Zone *zone = memManageStruct.zones + zId;
        u64 resLen = zone->pagesLength, pos = 0;
        // ignore the pages that used to store the data of global memory struct and buddy struct.
        while (pos < zone->pagesLength && (zone->pages[pos].attribute & PAGE_GloAttribute)) pos++, resLen--;
        for (int i = BUDDY_MAX_ORDER; i >= 0; i--) {
            while ((1ul << i) <= resLen) {
                Page *headPage = zone->pages + pos;
                headPage->attribute |= PAGE_HeadPage;
                headPage->posId = 1;
                *pageOrder(headPage) = i;
                if (!List_isEmpty((List *)buddyStruct.freePageList[i]))
                    ((Page *)buddyStruct.freePageList[i]->listEle.next)->attribute ^= PAGE_HeadPage;
                List_addBehind((List *)buddyStruct.freePageList[i], (List *)headPage);
                resLen -= (1ul << i), pos += (1ul << i);
            }
        }
    }
}

Page *Buddy_alloc(u64 log2Size, u64 attribute) {
    if (log2Size > BUDDY_MAX_ORDER) return NULL;
    Page *headPage = NULL;
    for (int i = log2Size; i <= BUDDY_MAX_ORDER; i++) {
        if (List_isEmpty((List *)buddyStruct.freePageList[i]))
            continue;
        Page *headPage = (Page *)buddyStruct.freePageList[i]->listEle.next;
        delFreePage(headPage);
        revBit(headPage, i);
        // seperate it into two part
        for (int ord = i; ord > log2Size; ord--) {
            Page *rPart = headPage + (1ul << (ord - 1));
            *pageOrder(rPart) = ord - 1;
            rPart->posId = rightPosId(headPage->posId);
            addPage2FreeList(rPart, ord - 1);
            headPage->posId = leftPosId(headPage->posId);
            (*pageOrder(headPage))--;
        }
        break;
    }
    if (headPage != NULL) {

    }
    return headPage;
}

void Buddy_free(Page *page) {
    if (page == NULL) return ;
    addPage2FreeList(page, *pageOrder(page));
}

void Buddy_debug() {
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        printk(WHITE, BLACK, "Order %d: ", i);
        for (List *p = buddyStruct.freePageList[i]->listEle.next; p != (List *)buddyStruct.freePageList[i]; p = p->next) {
            printk(RED, BLACK, "%p ", ((Page *)p));
        }
        putchar(RED, BLACK, '\n');
    }
}