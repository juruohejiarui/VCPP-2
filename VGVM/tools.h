#pragma once

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdarg.h>
#include "../VM/tools.h"

void Debug_printJITCode(uint8 *instBlk, uint64 size);
void Debug_saveJITCode(uint8 *instBlk, uint64 size, char *logPath);