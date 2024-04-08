#ifndef __LOG_H__
#define __LOG_H__
#include "linkage.h"
#include <stdarg.h>

#define RED     0x00ff0000
#define WHITE   0x00ffffff
#define BLUE    0x000000ff
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
#endif