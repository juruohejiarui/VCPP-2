#include "message.h"

void setColor(int _type) {
    int _c = 0;
    switch (_type) {
        case messageTypeError:
            _c = 0x01; break;
        case messageTypeWarning:
            _c = 0x03; break;
        case messageTypeLog:
            _c = 0x07; break;
    }
    printf("\e[3%cm", _c + '0');
}

void printMessage(const char *_message, int _type) {
    setColor(_type), printf("%s", _message);
}