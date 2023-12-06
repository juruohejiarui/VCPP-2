#pragma once
#include <stdio.h>

enum messageType {
    messageTypeError, messageTypeWarning, messageTypeLog
};
void printMessage(const char *_message, int _type);