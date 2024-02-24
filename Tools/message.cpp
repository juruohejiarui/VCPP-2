#include "message.h"
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#elif __APPLE__
#include<unistd.h>
#elif __linux__
#include<unistd.h>
#endif

#ifdef __linux__
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
#elif __APPLE__
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
#elif _WIN32
void setColor(UINT uFore, UINT uBack) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, uFore + uBack * 0x10);
}
void setColor(MessageType type) {
    switch (type) {
    case MessageType::Error:
        setColor(FOREGROUND_RED, 0);
        break;
    case MessageType::Log:
        setColor(0x07, 0x00);
        break;
    case MessageType::Warning:
        setColor(0x03, 0x00);
        break;
    }
}
#endif

void printMessage(const char* msg, MessageType type) {
    setColor(type), printf("%s", msg), setColor(MessageType::Log);
}

void printMessage(const std::string& msg, MessageType type) {
    printMessage(msg.c_str(), type);
}

void printError(int lineId, const std::string& msg) {
    printMessage("Line " + std::to_string(lineId) + " " + msg + "\n", MessageType::Error);
}

std::string getIndent(int dep) {
    std::string res = "";
    while (dep--) res += "  ";
    return res;
}

std::string getCwd() {
    char runPath[1024] = { 0 };
    getcwd(runPath, sizeof(runPath));
    return std::string(runPath);
}
