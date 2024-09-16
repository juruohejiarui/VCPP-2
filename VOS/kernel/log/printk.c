#include <stdarg.h>
#include "../includes/log.h"
#include "../includes/lib.h"
#include "../includes/hardware.h"
#include "../includes/memory.h"
#include "../includes/task.h"

#include "font.h"

static u32 lineLength[4096] = { [0 ... 4095] = 0 };

static unsigned int *_bufAddr;
static SpinLock _printLock, _bufLock;

void Log_enableBuf() {
    u64 pixelSize = HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution * HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * sizeof(u32);
    _bufAddr = DMAS_phys2Virt(MM_Buddy_alloc(max(log2Ceil(pixelSize) - 12, 0), Page_Flag_Active | Page_Flag_Kernel | Page_Flag_KernelShare)->phyAddr);
    memcpy(position.FBAddr, _bufAddr, pixelSize);
}

void Log_init() {
    _bufAddr = NULL;
    memset(lineLength, 0, sizeof(lineLength));
	SpinLock_init(&_printLock);
    SpinLock_init(&_bufLock);

	position.XResolution = HW_UEFI_bootParamInfo->graphicsInfo.HorizontalResolution & 0xffff;
	position.YResolution = HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution & 0xffff;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = DMAS_phys2Virt(HW_UEFI_bootParamInfo->graphicsInfo.FrameBufferBase);
}

#define isDigit(ch) ((ch) >= '0' && (ch) <= '9')

static const unsigned int flag_fill_zero = 0x01, 
                            flag_left = 0x02, 
                            flag_space = 0x04, 
                            flag_sign = 0x08, 
                            flag_special = 0x10;

static int _skipAtoI(const char **fmt) {
    int i = 0, sign = 1;
    if (**fmt == '-') sign = -1, (*fmt)++;
    while (isDigit(**fmt)) {
        i = i * 10 + *((*fmt)++) - '0';
    }
    return i * sign;
}

#define do_div(n, base) ({ \
    int __res; \
    __asm__("divq %%rcx" : "=a" (n), "=d" (__res) : "0" (n), "1" (0), "c" (base)); \
    __res; \
})

static char *_number(char *str, i64 num, int base, int size, int precision, int type) {
    char c, sign, tmp[66];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    if (type & flag_left) type &= ~flag_fill_zero;
    if (base < 2 || base > 36) return 0;
    c = (type & flag_fill_zero) ? '0' : ' ';
    sign = 0;
    if (type & flag_sign) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & flag_space) {
            sign = ' ';
            size--;
        }
    }
    if (type & flag_special) {
        if (base == 16) {
            *str++ = '0';
            *str++ = 'x';
            size -= 2;
        } else if (base == 8) *str++ = '0', size--;
    }
    i = 0;
    if (num == 0) tmp[i++] = '0';
    else while (num != 0) tmp[i++] = digits[do_div(num, base)];
    if (i > precision) precision = i;
    size -= precision;
    if (!(type & (flag_left | flag_fill_zero))) while (size-- > 0) *str++ = ' ';
    if (sign) *str++ = sign;
    if (!(type & flag_left)) while (size-- > 0) *str++ = c;
    while (i < precision--) *str++ = '0';
    while (i-- > 0) *str++ = tmp[i];
    while (size-- > 0) *str++ = ' ';
    return str;
}
// get the string using FMT and ARGS, output to BUF and return the length of the output
int sprintf(char *buf, const char *fmt, va_list args) {
    char *str = buf, *s;
    int flags, len, qlf, i, fld_w, prec;
    while (*fmt != '\0') {
        if (*fmt != '%') {
            *(str++) = *(fmt++);
        } else {
            fmt++, flags = 0, len = 0;
            if (*fmt == '%') {
                *(str++) = *(fmt++);
                continue;
            }
            scanFlags:
            switch (*fmt) {
                case '0': flags |= flag_fill_zero; fmt++; goto scanFlags;
                case '-': flags |= flag_left; fmt++; goto scanFlags;
                case ' ': flags |= flag_space; fmt++; goto scanFlags;
                case '+': flags |= flag_sign; fmt++; goto scanFlags;
                case '#': flags |= flag_special; fmt++; goto scanFlags;
                default: goto endScanFlags;
            }
            endScanFlags:
            fld_w = -1, qlf = 0;
            if (isDigit(*fmt))
                fld_w = _skipAtoI(&fmt);
            else if (*fmt == '*') {
                fld_w = va_arg(args, int);
                fmt++;
                if (fld_w < 0)
                    fld_w = -fld_w, flags |= flag_left;
            }
            prec = -1;
            if (*fmt == '.') {
                fmt++;
                if (isDigit(*fmt))
                    prec = _skipAtoI(&fmt);
                else if (*fmt == '*') {
                    prec = va_arg(args, int);
                    fmt++;
                }
                if (prec < 0) prec = 0;
            }
            switch (*fmt) {
                case 'h': qlf = (*(fmt + 1) == 'h' ? (fmt += 2, 'H') : (fmt++, 'h')); break;
                case 'l': qlf = (*(fmt + 1) == 'l' ? (fmt += 2, 'L') : (fmt++, 'L')); break;
                case 'j': qlf = (fmt++, 'j'); break;
                case 'z': qlf = (fmt++, 'z'); break;
                case 't': qlf = (fmt++, 't'); break;
                case 'L': qlf = (fmt++, 'L'); break;
                default: qlf = 0; break;
            }
            switch (*fmt) {
                case 'c':
                    if (!(flags & flag_left))
                        while (--fld_w > 0) *(str++) = ' ';
                    *(str++) = (unsigned char)va_arg(args, int);
                    while (--fld_w > 0) *(str++) = ' ';
                    break;
                case 's':
                    s = va_arg(args, char *);
                    len = strlen(s);
                    if (prec >= 0 && len > prec) len = prec;
                    if (!(flags & flag_left))
                        while (len < fld_w--) *(str++) = ' ';
                    for (i = 0; i < len; i++) *(str++) = *(s + i);
                    while (len < fld_w--) *(str++) = ' ';
                    break;
                case 'o':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 8, fld_w, prec, flags);
                    break;
                case 'p':
                    if (fld_w == -1) {
                        fld_w = 2 * sizeof(void *);
                        flags |= flag_fill_zero;
                    }
                    str = _number(str, (unsigned long)va_arg(args, void *), 16, fld_w, prec, flags);
                    break;
                case 'x':
                case 'X':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 16, fld_w, prec, flags);
                    break;
                case 'd':
                case 'i':
                    flags |= flag_sign;
                    str = _number(str, qlf == 'L' ? va_arg(args, i64) : va_arg(args, i32), 10, fld_w, prec, flags);
					break;
                case 'u':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 10, fld_w, prec, flags);
                    break;
                default:
                    *(str++) = '%';
                    break;
            }
            *fmt++;
        }
    }
    *str = '\0';
    return str - buf;
}

static void _scroll(void) {
    int x, y;
    unsigned int *addr = position.FBAddr, 
                *addr2 = position.FBAddr + position.YCharSize * HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine,
                *bufAddr = _bufAddr,
                *bufAddr2 = _bufAddr + position.YCharSize * HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine;
    u64 offPerLine = HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * position.YCharSize;
    for (int i = 0; i < position.YPosition - 1; i++) {
        u32 size = max(lineLength[i + 1], lineLength[i]) * position.XCharSize * sizeof(u32);
        for (int j = 0, off = 0; j < position.YCharSize; j++, off += HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine) {
            if (_bufAddr == NULL) {
                memcpy(addr2 + off, addr + off, size);
            } else {
                memcpy(bufAddr2 + off, addr + off, size);
                memcpy(bufAddr2 + off, bufAddr + off, size);
            }
        }
        
        addr += offPerLine;       addr2 += offPerLine;
        bufAddr += offPerLine;    bufAddr2 += offPerLine;
        lineLength[i] = lineLength[i + 1];
    }
    memset(addr, 0, HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * position.YCharSize * sizeof(u32));
    if (_bufAddr != NULL) memset(bufAddr, 0, HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * position.YCharSize * sizeof(u32));
    lineLength[position.YPosition - 1] = 0;
}

static void _drawchar(unsigned int fcol, unsigned int bcol, int px, int py, char ch) {
    int x, y;
    int testVal; u64 off;
    unsigned int *addr, *bufAddr;
    unsigned char *fontp = font_ascii[ch];
        for (y = 0; y < position.YCharSize; y++, fontp++) {
            off = HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * (py + y) + px;
            addr = position.FBAddr + off;
            bufAddr = _bufAddr + off;
            testVal = 0x80;
            for (x = 0; x < position.XCharSize; x++, addr++, bufAddr++, testVal >>= 1) {
                *addr = ((*fontp & testVal) ? fcol : bcol);
                if (_bufAddr != NULL)
                    *bufAddr = ((*fontp & testVal) ? fcol : bcol);
            }
        }
}

inline void putchar(unsigned int fcol, unsigned int bcol, char ch) {
    int i;
    if (ch == '\n') {
        position.YPosition++, position.XPosition = 0;
        if (position.YPosition >= position.YResolution / position.YCharSize) {
            _scroll();
			position.YPosition--;
        }
    } else if (ch == '\r') {
        position.XPosition = 0;
    } else if (ch == '\b') {
        if (position.XPosition) position.XPosition--;
        else position.YPosition--, position.XPosition = position.XResolution / position.XCharSize;
    } else if (ch == '\t') {
        do {
            putchar(fcol, bcol, ' ');
        } while (position.XPosition & 3);
    } else {
        if (position.XPosition == position.XResolution / position.XCharSize)
            putchar(fcol, bcol, '\n');
        _drawchar(fcol, bcol, position.XCharSize * position.XPosition, position.YCharSize * position.YPosition, ch);
        position.XPosition++;
        lineLength[position.YPosition] = max(lineLength[position.YPosition], position.XPosition);
    }
}

void printStr(unsigned int fcol, unsigned int bcol, const char *str, int len) {
    // close the interrupt if it is open now
	u64 prevState = (IO_getRflags() >> 9) & 1;
	if (prevState) IO_cli();
	SpinLock_lock(&_printLock);
    while (len--) putchar(fcol, bcol, *str++);
	SpinLock_unlock(&_printLock);
    if (prevState) IO_sti();
}

void clearScreen() {
	u64 prevState = (IO_getRflags() >> 9) & 1;
	if (prevState) IO_cli();
    SpinLock_lock(&_printLock);
	memset(position.FBAddr, 0, (position.YPosition + 1) * position.YCharSize * HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * sizeof(u32));
   	if (_bufAddr != NULL)
		memset(_bufAddr, 0, (position.YPosition + 1) * position.YCharSize * HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine * sizeof(u32));
	memset(lineLength, 0, 4096 * sizeof(u32));
	position.XPosition = 0, position.YPosition = 0;
	SpinLock_unlock(&_printLock);
	if (prevState) IO_sti();
}

void printk(unsigned int fcol, unsigned int bcol, const char *fmt, ...) {
    // SpinLock_lock(&_bufLock);
    char buf[512] = {0};
    int len = 0, i;
    va_list args;
    va_start(args, fmt);
    len = sprintf(buf, fmt, args);
    va_end(args);
    if (Task_getRing() == 0) printStr(fcol, bcol, buf, len);
    else Task_Syscall_usrAPI(1, fcol, bcol, (u64)buf, len, 0, 0);
    // SpinLock_unlock(&_bufLock);
}

u64 Syscall_clearScreen(u64 _1, u64 _2, u64 _3, u64 _4, u64 _5) {
	clearScreen();
	return 0;
}
u64 Syscall_printStr(u64 fcol, u64 bcol, u64 str, u64 len, u64 _5) {
    printStr(fcol, bcol, (char *)str, len);
    return 0;
}