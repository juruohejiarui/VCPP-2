#include "ds.h"
#include "../includes/log.h"

void List_init(List *list) {
    list->prev = list->next = list;
}

void List_insBehind(List *ele, List *pos) {
    ele->next = pos->next, ele->prev = pos;
    pos->next->prev = ele;
    pos->next = ele;
}

void List_insBefore(List *ele, List *pos) {
    ele->next = pos, ele->prev = pos->prev;
    pos->prev->next = ele;
    pos->prev = ele;
}

int List_isEmpty(List *ele) { return ele->prev == ele && ele->next == ele; }

void List_del(List *ele) {
    ele->next->prev = ele->prev;
    ele->prev->next = ele->next;
    ele->prev = ele->next = ele;
}

u64 Bit_get(u64 *addr, u64 index) { return ((*addr) >> index) & 1; }
void Bit_set1(u64 *addr, u64 index) { *addr |= (1ul << index); }
void Bit_set0(u64 *addr, u64 index) { *addr &= (~(1ul << index)); }