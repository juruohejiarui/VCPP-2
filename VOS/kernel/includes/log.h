#ifndef __LOG_H__
#define __LOG_H__
#include "linkage.h"
#include <stdarg.h>
#include "lib.h"

#define RED     0x00ff0000
#define WHITE   0x00ffffff
#define BLUE    0x0000afff
#define GREEN   0x0000ff00
#define BLACK   0x00000000
#define ORANGE  0x00ff8000
#define YELLOW  0x00ffff00

struct Position {
    int XResolution, YResolution;
    int XPosition, YPosition;

    int XCharSize, YCharSize;

    unsigned int *FBAddr;
    unsigned int FBLength;
} position;

void putchar(unsigned int fcol, unsigned int bcol, char ch);

void printk(unsigned int fcol, unsigned int bcol, const char *fmt, ...);

u64 Syscall_printStr(u64 fcol, u64 bcol, u64 str, u64 len, u64 _5);
u64 Syscall_clearScreen(u64 _1, u64 _2, u64 _3, u64 _4, u64 _5);

void Log_enableBuf();
void Log_init();

#endif