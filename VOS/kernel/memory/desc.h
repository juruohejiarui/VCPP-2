// this file defines the initialization process of memory management and the APIs of basic management system
#ifndef __MEMORY_DESC_H__
#define __MEMORY_DESC_H__

#include "../includes/lib.h"

extern char _text;
extern char _etext;
extern char _code;
extern char _ecode;
extern char _bss;
extern char _ebss;
extern char _data;
extern char _edata;
extern char _end;

#define Page_4KShift 12
#define Page_2MShift 21
#define Page_1GShift 30

#define Page_4KSize (1ul << Page_4KShift)
#define Page_2MSize (1ul << Page_2MShift)
#define Page_1GSize (1ul << Page_1GShift)

#define Page_4KMask (Page_4KSize - 1)
#define Page_2MMask (Page_2MSize - 1)
#define Page_1GMask (Page_1GSize - 1)

#define Page_4KDownAlign(addr) ((addr) & (~Page_4KMask))
#define Page_2MDownAlign(addr) ((addr) & (~Page_2MMask))
#define Page_1GDownAlign(addr) ((addr) & (~Page_1GMask))
#define Page_4KUpAlign(addr) (((addr) + Page_4KMask) & (~Page_4KMask))
#define Page_2MUpAlign(addr) (((addr) + Page_2MMask) & (~Page_2MMask))
#define Page_1GUpAlign(addr) (((addr) + Page_1GMask) & (~Page_1GMask))


#define Page_Flag_Kernel        (1ul << 0)
#define Page_Flag_KernelInit    (1ul << 1)
#define Page_Flag_Active        (1ul << 2)
#define Page_Flag_ShareK2U      (1ul << 3)
#define Page_Flag_BuddyHeadPage (1ul << 4)



#define availVirtAddrSt ((u64 *)Page_4KUpAlign(0xffff800003000000ul + bootParamInfo->graphicsInfo.FrameBufferSize))

struct tmpPage {
    u64 phyAddr;
    u64 attr;
    u64 buddyId;
    List listEle;
    struct tmpZone *blgZone;
};
typedef struct tmpPage Page;

struct tmpZone {
    u64 phyAddrSt, phyAddrEd;
    Page *pages;
    u64 pagesLength;
    u64 attribute;
    u64 freeCnt, usingCnt;
};
typedef struct tmpZone Zone;

typedef struct {
    u64 addr;
    u64 size;
    u32 type;
} __attribute__((packed)) E820;

struct GlobalMemManageStruct {
    E820 e820[32];
    u32 e820Length;

    Page *pages;
    Zone *zones;
    u64 pagesLength;
    u64 zonesLength;
    u64 pagesSize;
    u64 zonesSize;

    u64 edOfStruct;
    u64 totMemSize;
};

extern struct GlobalMemManageStruct memManageStruct;

void Init_memManage();

void BsMemMange_setPageAttr(Page *page, u64 attr);
u64 BsMemManage_getPageAttr(Page *page);
Page *BsMemManage_alloc(u64 num, u64 attr);

#endif