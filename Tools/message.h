#pragma once
#include <stdio.h>
#include <string>

enum class MessageType {
    Error, Warning, Log
};
void printMessage(const char *_message, MessageType _type);
void printMessage(const std::string &_message, MessageType _type);

void printError(int _line, const std::string &_message);