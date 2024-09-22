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
	int lgMn, lgMx, id, cnt, sz;
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

static int (*modiChk[0x200])(struct DataState *, int, u8);
static int (*_modi[0x200])(struct DataState *, int isIn, u8 flags, USB_HID_ParseHelper *helper);

#define regModifer(modifier) \
	(modiChk[_modiNum] = (_modiChk_##modifier), _modi[_modiNum] = (_modi_##modifier), ++_modiNum)

static int _modiNum;

static void _applyItem(struct DataState *state, u8 flags, int isIn, USB_HID_ParseHelper *helper, USB_HID_ReportItem *item) {
	item->flags = flags;
	item->rgMn = state->lgMn;
	item->rgMx = state->lgMx;
	item->off = (isIn ? helper->inSz : helper->outSz);
	item->size = state->sz;
}

#pragma region Modifiers
static int _modiChk_mouseX(struct DataState *state, int isIn, u8 flags) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse)
				&& _countUsage(state, -1, HID_Usage_X)
				&& !HID_RepItem_Main_isConst(flags);
}
static int _modi_mouseX(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	helper->type = HID_RepItem_Main_isRel(flags) ? USB_HID_ReportHelper_Type_Mouse : USB_HID_ReportHelper_Type_Touchpad;
	printk(WHITE, BLACK, "mouseX off:%d\n", helper->inSz);
	_applyItem(state, flags, 1, helper, &helper->items.mouse.x);
	return 1;
}
static int _modiChk_mouseY(struct DataState *state, int isIn, u8 flags) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse)
				&& _countUsage(state, -1, HID_Usage_Y)
				&& !HID_RepItem_Main_isConst(flags);
}
static int _modi_mouseY(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	printk(WHITE, BLACK, "mouseY off:%d\n", helper->inSz);
	_applyItem(state, flags, 1, helper, &helper->items.mouse.y);
	return 1;
}
static int _modiChk_mouseWheel(struct DataState *state, int isIn, u8 flags) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse)
				&& _countUsage(state, -1, HID_Usage_Wheel)
				&& !HID_RepItem_Main_isConst(flags);
}
static int _modi_mouseWheel(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	printk(WHITE, BLACK, "mouse wheel off:%d\n", helper->inSz);
	_applyItem(state, flags, 1, helper, &helper->items.mouse.wheel);
	return 1;
}
static int _modiChk_mouseBtn(struct DataState *state, int isIn, u8 flags) {
	return isIn && _countUsage(state, 0, HID_Usage_Mouse)
				&& state->usagePg == HID_UsagePage_Button
				&& !HID_RepItem_Main_isConst(flags);
}
static int _modi_mouseBtn(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	_applyItem(state, flags, 1, helper, &helper->items.mouse.btn);
	printk(WHITE, BLACK, "mouse button off:%d\n", helper->inSz);
	helper->items.mouse.btn.size = state->cnt * state->sz;
	return state->cnt;
}
static int _modiChk_keyboard(struct DataState *state, int isIn, u8 flags) {
	return isIn && state->usagePg == HID_UsagePage_Keyboard && !HID_RepItem_Main_isConst(flags);
}
static int _modi_keyboard(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	helper->type = USB_HID_ReportHelper_Type_Keyboard;
	// ctrl keys
	if (state->sz == 1) {
		printk(WHITE, BLACK, "keyboard spK off:%d size:%d\n", helper->inSz, state->cnt * state->sz);
		_applyItem(state, flags, 1, helper, &helper->items.keyboard.spK);
		helper->items.keyboard.spK.off = helper->inSz;
		helper->items.keyboard.spK.size = state->cnt * state->sz;
	} else {
		printk(WHITE, BLACK, "keyboard key off:%d size:%d ", helper->inSz, state->sz);
		for (int i = 0; i < state->cnt; i++) {
			register USB_HID_ReportItem *key = &helper->items.keyboard.key[i];
			printk(WHITE, BLACK, "key %d: off:%d ", i, helper->inSz + i * state->sz);
			key->off = helper->inSz + i * state->sz;
			key->flags = flags;
			key->rgMn = state->lgMn, key->rgMx = state->lgMx;
			key->size = state->sz;
		}
		printk(WHITE, BLACK, "\n");
	}
	return state->cnt;
}
#pragma endregion

static void regDefaultModifier() {
	// the order of registration counts
	regModifer(mouseBtn);
	regModifer(mouseX);
	regModifer(mouseY);
	regModifer(mouseWheel);

	regModifer(keyboard);
}

static void _tryModify(struct DataState *state, int isIn, u8 flags, USB_HID_ParseHelper *helper) {
	int uCnt = 0;
	for (int i = 0; i < _modiNum; i++) if (modiChk[i] && modiChk[i](state, isIn, flags)) {
		int used = _modi[i](state, isIn, flags, helper);
		if (isIn) helper->inSz += used * state->sz;
		else helper->outSz += used * state->sz;
		uCnt += used;
		if (uCnt == state->cnt) break;
	}
	// some unrecognizable items
	if (state->cnt) {
		if (isIn)	helper->inSz += state->sz * (state->cnt - uCnt);
		else		helper->outSz += state->sz * (state->cnt - uCnt);
	}
}

static int _parseMain(USB_HID_ParseHelper *helper, u8 *rep, struct DataState *state) {
	switch (_getPrefixField(*rep, HID_RepItem_Tag)) {
		case HID_RepItem_Tag_Input:
		case HID_RepItem_Tag_Output:
			printk(WHITE, BLACK, "main: locTop:%d lgMn:%d lgMx:%d cnt:%d sz:%d usgPg:%d flags:%d usage:",
				state->locTop, state->lgMn, state->lgMx, state->cnt, state->sz, state->usagePg, _getItemData(rep));
			for (int i = 0; i < state->usageNum[state->locTop]; i++)
				printk(WHITE, BLACK, "%x ", state->usage[state->locTop][i]);
			printk(WHITE, BLACK, "\n");
			_tryModify(state, (_getPrefixField(*rep, HID_RepItem_Tag) == HID_RepItem_Tag_Input), *(rep + 1), helper);
			state->usageNum[state->locTop] = 0;
			break;
		case HID_RepItem_Tag_EndColl:
			state->locTop--;
			break;
		case HID_RepItem_Tag_Feature:
			// no support for this tag;
			break;
		case HID_RepItem_Tag_Coll:
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
		case HID_RepItem_Tag_LogicalMin: {
			int data = state->lgMn = _getItemData(rep);
			break;
		}
		case HID_RepItem_Tag_LogicalMax:
			state->lgMx = _getItemData(rep);
			break;
		case HID_RepItem_Tag_ReportCnt:
			state->cnt = _getItemData(rep);
			break;
		case HID_RepItem_Tag_ReportSize:
			state->sz = _getItemData(rep);
			break;
		case HID_RepItem_Tag_ReportId:
			state->id = _getItemData(rep);
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

static SpinLock _lock;

USB_HID_ParseHelper *HW_USB_HID_genParseHelper(u8 *rep, u64 len) {
	SpinLock_lock(&_lock);
	u64 idx = 0;
	int res;
	USB_HID_ParseHelper *helper = kmalloc(sizeof(USB_HID_ParseHelper), Slab_Flag_Private | Slab_Flag_Clear, NULL);
	static struct DataState curDtState;
	memset(&curDtState, 0, sizeof(struct DataState));
	while (idx < len) {
		int itemLen = _getItemSize((rep + idx));
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
			for (int i = 0; i < len; i++) printk(WHITE, BLACK, "%02x ", rep[i]);
			printk(WHITE, BLACK, "\n");
			SpinLock_unlock(&_lock);
			return helper;
		}
		idx += 1 + itemLen;
	}
	helper->id = curDtState.id;
	SpinLock_unlock(&_lock);
	printk(WHITE, BLACK, "HID parse helper: %#018lx: reportId:%d type:%d inSz:%d outSz:%d\n", helper, helper->id, helper->type, helper->inSz, helper->outSz);
	return helper;
}

static void _parseItem(u8 *raw, USB_HID_ReportItem *item, int *out) {
	register u32 msk = ((1ul << item->size) - 1) << (item->off & 0x7);
	u32 data = ((*(u32 *)(raw + (item->off / 8))) & msk) >> (Bit_ffs(msk) - 1);
	// extend the sign bit if necessary
	if (data & (1u << (item->size - 1)) && HID_RepItem_Main_isRel(item->flags))
		data |= ~((1ul << item->size) - 1u);
	*out = data;
}

void HW_USB_HID_parseReport(u8 *raw, USB_HID_ParseHelper *helper, USB_HID_Report *out) {
	out->type = helper->type;
	switch (helper->type) {
		case USB_HID_ReportHelper_Type_Mouse:
			_parseItem(raw, &helper->items.mouse.btn, &out->items.mouse.btn);
			_parseItem(raw, &helper->items.mouse.x, &out->items.mouse.x);
			_parseItem(raw, &helper->items.mouse.y, &out->items.mouse.y);
			_parseItem(raw, &helper->items.mouse.wheel, &out->items.mouse.wheel);
			break;
		case USB_HID_ReportHelper_Type_Keyboard:
			_parseItem(raw, &helper->items.keyboard.spK, &out->items.keyboard.spK);
			for (int i = 0; i < 6; i++) _parseItem(raw, &helper->items.keyboard.key[i], &out->items.keyboard.key[i]);
			break;
	}
}

int HW_USB_HID_getIdleDuration(int repType) {
	switch (repType) {
		case USB_HID_ReportHelper_Type_Mouse : return 0x00;
		case USB_HID_ReportHelper_Type_Keyboard : return 0x0;
	}
	return 0;
}

void HW_USB_HID_initParse() {
	SpinLock_init(&_lock);
	_modiNum = 0;
	regDefaultModifier();
}