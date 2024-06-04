#include "keyboard.h"
#include "../includes/memory.h"
#include "../includes/interrupt.h"
#include "../includes/log.h"
#include "APIC.h"

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
	printk(WHITE, BLACK, "(K:%02x)", x);
	if (_buffer->tail == _buffer->data + HW_Keyboard_BufferSize)
		_buffer->tail = _buffer->data;
	*_buffer->tail = x;
	_buffer->tail++;
	_buffer->size++;
}

u8 _getKeyCode() {
	u8 res = *_buffer->head;
	_buffer->head++;
	_buffer->size--;
	if (_buffer->head == _buffer->data + HW_Keyboard_BufferSize)
		_buffer->head = _buffer->data;
}

KeyboardEvent *HW_Keyboard_getEvent() {
    if (_buffer->size == 0) return NULL;
	KeyboardEvent *e = (KeyboardEvent *)kmalloc(sizeof(KeyboardEvent), 0);
	u8 fir = _getKeyCode();
	
}

void HW_Keyboard_init()
{
    _buffer = (struct KeyboardBuffer *)kmalloc(sizeof(struct KeyboardBuffer), 0);
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