#include "api.h"
#include "../../../includes/log.h"

static __always_inline__ int _getPrefixField(u8 prefix, u32 mask) { return (prefix & mask) >> (Bit_ffs(mask) - 1); }
static __always_inline__ int _getItemSize(u8 *rep) {
    u8 *szField = _getPrefixField(*rep, HID_RepItem_Size);
    return szField + (szField == 3 ? 1 : 0);
}

struct DataState {
	// some states from global items
    int lgMn, lgMx, phyMn, phyMx;
    int cnt, sz;
	u8 usagePg;
	// some states from local items
    u64 usage[16][4], usageTop;  
};

static __always_inline__ int _usage(u64 usage[4], int Id) { return (usage[Id / 64] >> (Id % 64)) & 1; }

static int (*_modifierChk[0x200])(struct DataState *);
static void (*_modifier[0x200])(struct DataState *, u8 flags, USB_HID_ReportHelper *helper);

static int _modifierNum;

static void _applyItem(struct DataState *state, u8 flags, int isIn, USB_HID_ReportHelper *helper, USB_HID_ReportItem *item) {
    item->flags = flags;
    item->rgMn = state->lgMn;
    item->rgMx = state->lgMx;
    if (isIn) item->off = helper->inSz, helper->inSz += state->sz;
    else item->off = helper->outSz, helper->outSz += state->sz;
}

static void _tryModify(struct DataState *state, u8 flags, USB_HID_ReportHelper *helper) {
	for (int i = 0; i < _modifierNum; i++) if (_modifierChk[i] && _modifierChk[i](state)) {
		_modifier[i](state, flags, helper);
		break;
	}
}

int _parseMain(USB_HID_ReportHelper *helper, u8 *rep, struct DataState *state) {
    switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
        case HID_RepItem_Tag_Input:
        case HID_RepItem_Tag_Output:
            _tryModify(*(rep + 1), state, helper);
		case HID_RepItem_Tag_EndColl:
			memset(&state->usage[--state->usageTop], 0, sizeof(u64) * 4);
            break;
        case HID_RepItem_Tag_Feature:
            // no support for this tag;
            break;
		case HID_RepItem_Tag_Coll:
			state->usageTop++;
			break;
    }
	return 0;
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