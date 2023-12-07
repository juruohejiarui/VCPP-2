#include "message.h"

void setColor(MessageType _type) {
    int _c = 0;
    switch (_type) {
        case MessageType::Error:
            _c = 0x01; break;
        case MessageType::Warning:
            _c = 0x03; break;
        case MessageType::Log:
            _c = 0x07; break;
    }
    printf("\e[3%cm", _c + '0');
}

void printMessage(const char *_message, MessageType _type) {
    setColor(_type), printf("%s\e[0m", _message);
}

void printMessage(const std::string &_message, MessageType _type) {
    printMessage(_message.c_str(), _type);
}

void printError(int _line, const std::string &_message) {
    printMessage("Line " + std::to_string(_line) + " " + _message, MessageType::Error);
}