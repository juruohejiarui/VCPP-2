#include "api.h"
#include "../../../includes/log.h"

static __always_inline__ int _getPrefixField(u8 prefix, u32 mask) { return (prefix & mask) >> (Bit_ffs(mask) - 1); }
static __always_inline__ int _getItemSize(u8 *rep) {
	u8 szField = _getPrefixField(*rep, HID_RepItem_Size);
	return szField + (szField == 3 ? 1 : 0);
}
static __always_inline__ int _getItemData(u8 *rep) {
	int data = 0;
	switch (_getItemSize(rep)) {
		case 1:
			data = (int)*(i8 *)(rep + 1);
			break;
		case 2:
			data = (int)*(i16 *)(rep + 1);
			break;
		case 4:
			data = *(i32 *)(rep + 1);
			break;
	}
	return data;
}

struct DataState {
	// some states from global items
	int lgMn, lgMx, cnt, sz;
	u8 usagePg;
	// some states from local items
	int usage[8][0x10], phyMn[8], phyMx[8], locTop, usageNum[8];  
};

static int _countUsage(struct DataState *state, int rgSt, int usage) {
	if (rgSt < 0) rgSt = state->locTop + rgSt + 1;
	for (int i = rgSt; i <= state->locTop; i++) 
		for (int j = 0; j < state->usageNum[i]; j++)
			if (state->usage[i][j] == usage) return 1;
	return 0;
}

static int (*modiChk[0x200])(struct DataState *, int isIn);
static void (*_modi[0x200])(struct DataState *, int isIn, u8 flags, USB_HID_ReportHelper *helper);

#define regModifer(modifier) \
	(modiChk[_modiNum] = (_modiChk_##modifier), _modi[_modiNum] = (_modi_##modifier), ++_modiNum)

static int _modiNum;

static void _applyItem(struct DataState *state, u8 flags, int isIn, USB_HID_ReportHelper *helper, USB_HID_ReportItem *item) {
	item->flags = flags;
	item->rgMn = state->lgMn;
	item->rgMx = state->lgMx;
	item->off = (isIn ? helper->inSz : helper->outSz);
}

#pragma region Modifiers
static int _modiChk_mouseX(struct DataState *state, int isIn) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse) && _countUsage(state, -1, HID_Usage_X);
}
static void _modi_mouseX(struct DataState *state, int isIn, u8 flags, USB_HID_ReportHelper *helper) {
	helper->type = USB_HID_ReportHelper_Type_Mouse;
	_applyItem(state, flags, 1, helper, &helper->items.mouse.x);
}
static int _modiChk_mouseY(struct DataState *state, int isIn) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse) && _countUsage(state, -1, HID_Usage_Y);
}
static void _modi_mouseY(struct DataState *state, int isIn, u8 flags, USB_HID_ReportHelper *helper) {
	_applyItem(state, flags, 1, helper, &helper->items.mouse.y);
}
static int _modiChk_mouseWheel(struct DataState *state, int isIn) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse) && _countUsage(state, -1, HID_Usage_Wheel);
}
static void _modi_mouseWheel(struct DataState *state, int isIn, u8 flags, USB_HID_ReportHelper *helper) {
	_applyItem(state, flags, 1, helper, &helper->items.mouse.wheel);
}
static int _modiChk_mouseBtn(struct DataState *state, int isIn) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse) && (state->usagePg == HID_UsagePage_Button);
}
static void _modi_mouseBtn(struct DataState *state, int isIn, u8 flags, USB_HID_ReportHelper *helper) {
	_applyItem(state, flags, 1, helper, &helper->items.mouse.btn);
}
#pragma endregion

static void regDefaultModifier() {
	regModifer(mouseX);
	regModifer(mouseY);
	regModifer(mouseWheel);
	regModifer(mouseBtn);
}

static void _tryModify(struct DataState *state, int isIn, u8 flags, USB_HID_ReportHelper *helper) {
	for (int i = 0; i < _modiNum; i++) if (modiChk[i] && modiChk[i](state, isIn)) {
		_modi[i](state, isIn, flags, helper);
		break;
	}
	if (isIn)	helper->inSz += state->sz * state->cnt;
	else		helper->outSz += state->sz * state->cnt;
}

static int _parseMain(USB_HID_ReportHelper *helper, u8 *rep, struct DataState *state) {
	switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
		case HID_RepItem_Tag_Input:
		case HID_RepItem_Tag_Output:
			printk(WHITE, BLACK, "main: locTop:%d lgMn:%d lgMx:%d cnt:%d sz:%d usgPg:%d usage:",
				state->locTop, state->lgMn, state->lgMx, state->cnt, state->sz, state->usagePg);
			for (int i = 0; i < state->usageNum[state->locTop]; i++)
				printk(WHITE, BLACK, "%x ", state->usage[state->locTop][i]);
			printk(WHITE, BLACK, "\n");
			_tryModify(state, (_getPrefixField(*rep, HID_RepItem_Tag) == HID_RepItem_Tag_Input), *(rep + 1), helper);
			state->usageNum[state->locTop] = 0;
			break;
		case HID_RepItem_Tag_EndColl:
			printk(WHITE, BLACK, "End Collection\n");
			state->locTop--;
			break;
		case HID_RepItem_Tag_Feature:
			// no support for this tag;
			break;
		case HID_RepItem_Tag_Coll:
			printk(WHITE, BLACK, "Collection\n");
			state->locTop++;
			break;
		default:
			printk(RED, BLACK, "HID parse helper: invalid main item tag:%x\n", _getPrefixField(*rep, HID_RepItem_Tag));
			return 0;
	}
	return 1;
}

static int _parseGlobal(u8 *rep, struct DataState *state) {
	switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
		case HID_RepItem_Tag_UsagePage:
			state->usagePg = _getItemData(rep);
			break;
		case HID_RepItem_Tag_LogicalMin:
			state->lgMn = _getItemData(rep);
			break;
		case HID_RepItem_Tag_LogicalMax:
			state->lgMx = _getItemData(rep);
			break;
		case HID_RepItem_Tag_ReportCnt:
			state->cnt = _getItemData(rep);
			break;
		case HID_RepItem_Tag_ReportSize:
			state->sz = _getItemData(rep);
			break;
		default:
			printk(RED, BLACK, "HID parse helper: invalid global item tag:%x\n", _getPrefixField(*rep, HID_RepItem_Tag));
			return 0;
	}
	return 1;
}

static int _parseLocal(u8 *rep, struct DataState *state) {
	switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
		case HID_RepItem_Tag_UsageMin:
			state->phyMn[state->locTop] = _getItemData(rep);
			break;
		case HID_RepItem_Tag_UsageMax:
			state->phyMx[state->locTop] = _getItemData(rep);
			break;
		case HID_RepItem_Tag_Usage:
			state->usage[state->locTop][state->usageNum[state->locTop]++] = _getItemData(rep);
			break;
		default:
			printk(RED, BLACK, "HID parse helper: invalid local item tag:%x\n", _getPrefixField(*rep, HID_RepItem_Tag));
			return 0;
	}
	return 1;
}

USB_HID_ReportHelper *HW_USB_HID_genParseHelper(u8 *rep, u64 len) {
	u64 idx = 0;
	int res;
	USB_HID_ReportHelper *helper = kmalloc(sizeof(USB_HID_ReportHelper), Slab_kmalloc_arg_Private | Slab_kmalloc_arg_Clear, NULL);
	struct DataState curDtState;
	memset(&curDtState, 0, sizeof(struct DataState));
	while (idx < len) {
		int itemLen = _getItemSize((rep + idx));
		printk(YELLOW, BLACK, "idx:%d ", idx);
		switch (_getPrefixField(*(rep + idx), HID_RepItem_Type)) {
			case HID_RepItem_Type_Main:
				res = _parseMain(helper, rep + idx, &curDtState);
				break;
			case HID_RepItem_Type_Global:
				res = _parseGlobal(rep + idx, &curDtState);
				break;
			case HID_RepItem_Type_Local:
				res = _parseLocal(rep + idx, &curDtState);
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
	printk(WHITE, BLACK, "HID parse helper: %#018lx: type:%d inSz:%d outSz:%d\n", helper, helper->type, helper->inSz, helper->outSz);
	return helper;
}


void HW_USB_HID_initParse() {
	_modiNum = 0;
	regDefaultModifier();
}