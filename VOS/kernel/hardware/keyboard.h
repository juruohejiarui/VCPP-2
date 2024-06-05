#ifndef __HAREWARE_KEYBOARD_H__
#define __HAREWARE_KEYBOARD_H__

#include "../includes/lib.h"

#define HW_Keyboard_BufferSize  0x100

typedef struct {
    u8 isKeyUp;
    u8 isCtrlKey;
    // isCtrlKey=1: a ctrl key code; =0: a normal code, represented by ascii
    u8 keyCode;
} KeyboardEvent;

#define HW_Keyboard_Esc         0x00
#define HW_Keyboard_CtrlL       0x01
#define HW_Keyboard_CtrlR       0x21
#define HW_Keyboard_AltL        0x02
#define HW_Keyboard_AltR        0x22
#define HW_Keyboard_SuperL      0x03
#define HW_Keyboard_SuperR      0x23
#define HW_Keyboard_ShiftL      0x04
#define HW_Keyboard_ShiftR      0x24
#define HW_Keyboard_Enter       0x05
#define HW_Keyboard_CapsLock    0x06
#define HW_Keyboard_Delete      0x07
#define HW_Keyboard_Up          0x08
#define HW_Keyboard_Down        0x09
#define HW_Keyboard_Left        0x0a
#define HW_Keyboard_Right       0x0b
#define HW_Keyboard_Fn          0x0c
#define HW_Keyboard_F1          0x11
#define HW_Keyboard_F2          0x12
#define HW_Keyboard_F3          0x13
#define HW_Keyboard_F4          0x14
#define HW_Keyboard_F5          0x15
#define HW_Keyboard_F6          0x16
#define HW_Keyboard_F7          0x17
#define HW_Keyboard_F8          0x18
#define HW_Keyboard_F9          0x19
#define HW_Keyboard_F10         0x1a
#define HW_Keyboard_F11         0x1b
#define HW_Keyboard_F12         0x1c

// a function for finding the character in ascii by a normal key code and the state of 'shift'
char HW_Keyboard_getChar(u8 keyCode, u8 isShiftDown);

KeyboardEvent *HW_Keyboard_getEvent();

void HW_Keyboard_init();

#endif