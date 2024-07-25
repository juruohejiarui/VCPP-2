#include "ds.h"
#include "../includes/log.h"

__always_inline__ void List_init(List *list) {
    list->prev = list->next = list;
}

__always_inline__ void List_insBehind(List *ele, List *pos) {
    ele->next = pos->next, ele->prev = pos;
    pos->next->prev = ele;
    pos->next = ele;
}

__always_inline__ void List_insBefore(List *ele, List *pos) {
    ele->next = pos, ele->prev = pos->prev;
    pos->prev->next = ele;
    pos->prev = ele;
}

__always_inline__ int List_isEmpty(List *ele) { return ele->prev == ele && ele->next == ele; }

void List_del(List *ele) {
    ele->next->prev = ele->prev;
    ele->prev->next = ele->next;
    ele->prev = ele->next = ele;
}

__always_inline__ u64 Bit_get(u64 *addr, u64 index) { return ((*addr) >> index) & 1; }
__always_inline__ void Bit_set1(u64 *addr, u64 index) { *addr |= (1ul << index); }
__always_inline__ void Bit_set0(u64 *addr, u64 index) { *addr &= (~(1ul << index)); }
__always_inline__ void Bit_rev(u64 *addr, u64 index) {
    __asm__ volatile (
        "btsq %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}
