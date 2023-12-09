#include "message.h"

void setColor(MessageType type) {
    int _c = 0;
    switch (type) {
        case MessageType::Error:
            _c = 0x01; break;
        case MessageType::Warning:
            _c = 0x03; break;
        case MessageType::Log:
            _c = 0x07; break;
    }
    printf("\e[3%cm", _c + '0');
}

void printMessage(const char *msg, MessageType type) {
    setColor(type), printf("%s\e[0m", msg);
}

void printMessage(const std::string &msg, MessageType type) {
    printMessage(msg.c_str(), type);
}

void printError(int lineId, const std::string &msg) {
    printMessage("Line " + std::to_string(lineId) + " " + msg + "\n", MessageType::Error);
}