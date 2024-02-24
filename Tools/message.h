#pragma once
#include <stdio.h>
#include <string>

enum class MessageType {
    Error, Warning, Log
};
void printMessage(const char *msg, MessageType type);
void printMessage(const std::string &msg, MessageType type);

void printError(int lineId, const std::string &msg);

std::string getIndent(int dep);

std::string getCwd();