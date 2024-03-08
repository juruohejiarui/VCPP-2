#include "../includes/memory.h"

#define BUDDY_MAX_ORDER 11

struct BuddyStruct {
    List *head[BUDDY_MAX_ORDER];
    List *freeHead[BUDDY_MAX_ORDER];

    u64 endOfStruct;
};