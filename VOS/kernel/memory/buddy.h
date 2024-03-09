#ifndef __BUDDY_H__
#define __BUDDY_H__

#include "../includes/memory.h"
#define BUDDY_MAX_ORDER 11

typedef struct tmpBuddyList {
    struct tmpBuddyList *prev, *next;
    Page *pageAddr;
} BuddyList;

struct BuddyStruct {
    BuddyList *head[BUDDY_MAX_ORDER];
    BuddyList *freeHead[BUDDY_MAX_ORDER];

    BuddyList *freeListEle;
};

extern struct BuddyStruct buddyManageStruct;
#endif