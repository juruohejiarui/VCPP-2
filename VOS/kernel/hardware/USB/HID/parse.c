#include "api.h"
#include "../../../includes/log.h"

static __always_inline__ int _getPrefixField(u8 prefix, u32 mask) { return (prefix & mask) >> (Bit_ffs(mask) - 1); }
static __always_inline__ int _getItemSize(u8 *rep) {
    u8 *szField = _getPrefixField(*rep, HID_RepItem_Size);
    return szField + (szField == 3 ? 1 : 0);
}

struct DataState {
    int lgMn, lgMx, phyMn, phyMx;
    int cnt, sz;
    u8 usageLs[16], usageLsL;  
};

static void (*_helperModifier[0x100])(struct DataState *, u8, USB_HID_ReportHelper *helper);

static void _applyItem(struct DataState *state, u8 flags, int isIn, USB_HID_ReportHelper *helper, USB_HID_ReportItem *item) {
    item->flags = flags;
    item->rgMn = state->lgMn;
    item->rgMx = state->lgMx;
    if (isIn) item->off = helper->inSz, helper->inSz += state->sz;
    else item->off = helper->outSz, helper->outSz += state->sz;
}

static void _helperModifier_X(struct DataState *state, u8 flags, USB_HID_ReportHelper *helper) {
    if (helper->type != USB_HID_ReportHelper_Type_Mouse) return ;
    _applyItem(state, flags, 1, helper, &helper->items.mouse.mvX);
}
static void _helperModifier_Y(struct DataState *state, u8 flags, USB_HID_ReportHelper *helper) {
    if (helper->type != USB_HID_ReportHelper_Type_Mouse) return ;
    _applyItem(state, flags, 1, helper, &helper->items.mouse.mvY);
}
static void _helperModifier_Z(struct DataState *state, u8 flags, USB_HID_ReportHelper *helper) {
    if (helper->type != USB_HID_ReportHelper_Type_Mouse) return ;
    _applyItem(state, flags, 1, helper, &helper->items.mouse.mvZ);
}
static void _helperModifer_btn(struct DataState *state, u8 flags, USB_HID_ReportHelper *helper) {
    if (helper->type == USB_HID_ReportHelper_Type_Mouse) {
        for (int i = 0; i < 3; i++) if (!helper->items.mouse.btn[i].size) {
            _applyItem(state, flags, 1, helper, &helper->items.mouse.btn[i]);
            break;
        }
    }
}

void _applyHelper(USB_HID_ReportHelper *helper, u8 flags, struct DataState *state) {
    
}

int *_parseMain(USB_HID_ReportHelper *helper, u8 *rep, struct DataState *state) {
    switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
        case HID_RepItem_Tag_Input:
        case HID_RepItem_Tag_Output:
            _createHelper(helper, *(rep + 1), state);
            break;
        case HID_RepItem_Tag_Feature:
            // no support for this tag;
            break;
        case HID_RepItem_Tag_Coll:
        
        case HID_RepItem_Tag_EndColl:

    }
}

USB_HID_ReportHelper *HW_USB_HID_genParseHelper(u8 *rep, u64 len) {
    u64 idx = 0;
    int res;
    USB_HID_ReportHelper *helper = kmalloc(sizeof(USB_HID_ReportHelper), Slab_kmalloc_arg_Private | Slab_kmalloc_arg_Clear, NULL);
    struct DataState curDtState;
    memset(&curDtState, 0, sizeof(struct DataState));
    while (idx < len) {
        int itemLen = _getItemSize(*(rep + idx));
        switch (_getPrefixField(*(rep + idx), HID_RepItem_Type)) {
            case HID_RepItem_Type_Main:
                res = _parseMain(helper, rep + idx, &curDtState);
                break;
            case HID_RepItem_Type_Global:
                break;
            case HID_RepItem_Type_Local:
                break;
            default :
                printk(RED, BLACK, "HID parse helper: No support for long item.\n");
                return helper;
        }
        if (!res) {
            helper->type = 0;
            return helper;
        }
        idx += 1 + itemLen;
    }
    return helper;
}
