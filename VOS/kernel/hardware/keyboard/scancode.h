#ifndef __HW_KEYBOARD_SCANCODE_H__
#define __HW_KEYBOARD_SCANCODE_H__

#include "../../includes/lib.h"

#define HW_Keyboard_ScanCodeNum 0x80
#define HW_Keyboard_ScanCodeCol 0x2

extern u8 HW_Keyboard_normapMap[HW_Keyboard_ScanCodeCol * HW_Keyboard_ScanCodeNum];

#endif