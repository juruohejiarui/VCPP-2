#include "api.h"
#include "../../../includes/log.h"

static u32 _getData(u8 *report, u64 itemSize) {
    u32 data = report[0];
    if (itemSize > 1) {
        data |= ((u16)report[1]) << 8;
        if (itemSize > 2) {
            data |= ((u32)report[2]) << 16;
            if (itemSize > 3) data |= ((u32)report[3]) << 24;
        }
    }
    return data;
}

void _parseMain(USB_HID_ReportHelper *helper, u32 tag, u32 data, u64 usageFlags, int curSize, int curCnt, int curMn, int curMx) {
    
}

USB_HID_ReportHelper *HW_USB_HID_genParseHelper(u8 *report, u64 len) {
    USB_HID_ReportHelper *helper = kmalloc(sizeof(USB_HID_ReportHelper), Slab_kmalloc_arg_Private | Slab_kmalloc_arg_Clear, NULL);
    helper->raw = report;
    u64 idx = 0;
    int curSize = 0, curCnt = 0;
    int curMn = -32768, curMx = 32768;
    u64 usageFlags;
    while (idx < len) {
        int itemType = report[idx] & HID_ReportItem_Tag;
        int itemSize = (report[idx] & HID_ReportItem_Size) ? (report[idx] & HID_ReportItem_Size) : 4;
        if (idx + itemSize > len) {
            printk(RED, BLACK, "HID: syntax error on report %#018lx\n", report);
            helper->type = 0;
            return helper;
        }
        u32 data = _getData(report + idx + 1, itemSize);
        switch (itemType & HID_ReportItem_Type) {
            case HID_ReportItem_Type_Main:
                _parseMain(helper, itemType, data, usageFlags, curSize, curCnt, curMn, curMx);
                break;
            case HID_ReportItem_Type_Global:
                break;
            case HID_ReportItem_Type_Local:
                break;
        }
    }
    return helper;
}