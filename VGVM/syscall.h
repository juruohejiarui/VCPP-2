#pragma once
#include "tools.h"

extern uint64 syscallArgCnt[];
extern void *syscall[];
void VMputchar(char ch);
char VMgetchar();
void printInt(int32 data);
int32 inputInt();
void printInt64(int64 data);
int64 inputInt64();
void printUInt64(uint64 data);
uint64 inputUInt64();
uint64 getTime();

void printFloat32(float32 data);