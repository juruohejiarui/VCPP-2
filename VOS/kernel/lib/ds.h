#ifndef __LIB_DS_H__
#define __LIB_DS_H__

typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long i64;
typedef int i32;
typedef short i16;
typedef char i8;

typedef struct tmpList {
    struct tmpList *next, *prev;
} List;

void List_init(List *list);
void List_insBehind(List *ele, List *pos);
void List_insBefore(List *ele, List *pos);
int List_isEmpty(List *list);
void List_del(List *list);
#endif