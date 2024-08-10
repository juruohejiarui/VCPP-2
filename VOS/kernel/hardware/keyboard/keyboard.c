#include "../keyboard.h"
#include "../../includes/memory.h"
#include "../../includes/interrupt.h"
#include "../../includes/log.h"
#include "../APIC.h"
#include "scancode.h"

#define _IrqId 0x21

#define _Port_Data      0x60
#define _Port_Status    0x64
#define _Port_Cmd       0x64

#define _Cmd_Write      0x60
#define _Cmd_Read       0x20

#define _Mode_Init      0x47

#define _State_IBF      0x02
#define _State_OBF      0x01

#define _waitWrite()	while (IO_in8(_Port_Status) & _State_IBF)
#define _waitRead()		while (IO_in8(_Port_Status) & _State_OBF)

static struct KeyboardBuffer {
    u8 data[HW_Keyboard_BufferSize];
    u8 *head, *tail;
    u32 size;
} *_buffer;

static struct {
	int ShiftL, ShiftR, CtrlL, CtrlR, AltL, AltR;
} _mkeyStatus;

IntrController _controller = {
	.enable = HW_APIC_enableIntr,
	.disable = HW_APIC_disableIntr,
	.install = HW_APIC_install,
	.uninstall = HW_APIC_uninstall,
	.ack = HW_APIC_edgeAck
};

IntrHandlerDeclare(HW_Keyboard_handler) {
	u8 x = IO_in8(_Port_Data);
	// printk(WHITE, BLACK, "(K:%02x)", x);
	if (_buffer->tail == _buffer->data + HW_Keyboard_BufferSize)
		_buffer->tail = _buffer->data;
	*_buffer->tail = x;
	_buffer->tail++;
	_buffer->size++;
}

u8 _getKeyCode() {
	while (_buffer->size == 0) IO_hlt();
	u8 res = *_buffer->head;
	_buffer->head++;
	_buffer->size--;
	if (_buffer->head == _buffer->data + HW_Keyboard_BufferSize)
		_buffer->head = _buffer->data;
	return res;
}



KeyboardEvent *HW_Keyboard_getEvent() {
    if (_buffer->size == 0) return NULL;
	KeyboardEvent *e = (KeyboardEvent *)kmalloc(sizeof(KeyboardEvent), 0, NULL);
	u8 fir = _getKeyCode();
	e->isCtrlKey = 1;
	e->isKeyUp = 0;
	if (fir == 0xE0) {
		u8 sec = _getKeyCode();
		if (sec & 0x80) e->isKeyUp = 1;
		switch (sec & 0x7f) {
			case 0x5b: e->keyCode = HW_Keyboard_SuperL; break;
			case 0x5c: e->keyCode = HW_Keyboard_SuperR; break;
			case 0x1d: e->keyCode = HW_Keyboard_CtrlR; 	break;
			case 0x38: e->keyCode = HW_Keyboard_AltR;	break;
			case 0x48: e->keyCode = HW_Keyboard_Up;		break;
			case 0x50: e->keyCode = HW_Keyboard_Down;	break;
			case 0x4b: e->keyCode = HW_Keyboard_Left;	break;
			case 0x4d: e->keyCode = HW_Keyboard_Right; 	break;
			case 0x63: e->keyCode = HW_Keyboard_Fn;		break;
			default:
				printk(RED, BLACK, "Keyboard: Invalid keycode : 0xE0, %#04x\n", sec);
				break;
		}
	} else {
		if (fir & 0x80) e->isKeyUp = 1;
		switch (fir & 0x7f) {
			case 0x01: e->keyCode = HW_Keyboard_Esc;		break;
			case 0x1d: e->keyCode = HW_Keyboard_CtrlL;		break;
			case 0x2a: e->keyCode = HW_Keyboard_ShiftL;		break;
			case 0x36: e->keyCode = HW_Keyboard_ShiftR;		break;
			case 0x38: e->keyCode = HW_Keyboard_AltL;		break;
			case 0x3a: e->keyCode = HW_Keyboard_CapsLock;	break;
			case 0x3b: e->keyCode = HW_Keyboard_F1;			break;
			case 0x3c: e->keyCode = HW_Keyboard_F2;			break;
			case 0x3d: e->keyCode = HW_Keyboard_F3;			break;
			case 0x3e: e->keyCode = HW_Keyboard_F4;			break;
			case 0x3f: e->keyCode = HW_Keyboard_F5;			break;
			case 0x40: e->keyCode = HW_Keyboard_F6;			break;
			case 0x41: e->keyCode = HW_Keyboard_F7;			break;
			case 0x42: e->keyCode = HW_Keyboard_F8;			break;
			case 0x43: e->keyCode = HW_Keyboard_F9;			break;
			case 0x44: e->keyCode = HW_Keyboard_F10;		break;
			case 0x57: e->keyCode = HW_Keyboard_F11;		break;
			case 0x58: e->keyCode = HW_Keyboard_F12;		break;
			default :
				e->isCtrlKey = 0;
				e->keyCode = HW_Keyboard_normapMap[(fir & 0x7f) * HW_Keyboard_ScanCodeCol];
				break;
		}
	}
	return e;
}

void HW_Keyboard_init() {
    _buffer = (struct KeyboardBuffer *)kmalloc(sizeof(struct KeyboardBuffer), 0, NULL);
	_buffer->head = _buffer->tail = _buffer->data;
	_buffer->size = 0;
	memset(_buffer->data, 0, HW_Keyboard_BufferSize);

	APICRteDescriptor desc;
	desc.vector				= _IrqId;
	desc.deliveryMode		= HW_APIC_DeliveryMode_Fixed;
	desc.destMode			= HW_APIC_DestMode_Physical;
	desc.deliveryStatus		= HW_APIC_DeliveryStatus_Idle;
	desc.pinPolarity		= HW_APIC_PinPolarity_High;
	desc.remoteIRR			= HW_APIC_RemoteIRR_Reset;
	desc.triggerMode		= HW_APIC_TriggerMode_Edge;
	desc.mask				= HW_APIC_Mask_Masked;
	desc.reserved = 0;
	
	desc.destDesc.physical.destination = 0;
	desc.destDesc.physical.reserved2 = 0;
	desc.destDesc.physical.reserved3 = 0;

	_waitWrite();
	IO_out8(_Port_Cmd, _Cmd_Write);
	_waitWrite();
	IO_out8(_Port_Data, _Mode_Init);

	for (int i = 0; i < 1000; i++)
		for (int j = 0; j < 1000; j++) IO_nop();
	
	memset(&_mkeyStatus, 0, sizeof(_mkeyStatus));

	Intr_register(_IrqId, &desc, HW_Keyboard_handler, (u64)_buffer, &_controller, "PS/2 Keyboard");
}