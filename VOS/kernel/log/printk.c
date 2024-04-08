#include <stdarg.h>
#include "../includes/log.h"
#include "../includes/lib.h"
#include "font.h"

char buf[4096] = {0};

#define isDigit(ch) ((ch) >= '0' && (ch) <= '9')

static const unsigned int flag_fill_zero = 0x01, 
                            flag_left = 0x02, 
                            flag_space = 0x04, 
                            flag_sign = 0x08, 
                            flag_special = 0x10;

int skipAtoI(const char **fmt) {
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

char *number(char *str, i64 num, int base, int size, int precision, int type) {
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
                fld_w = skipAtoI(&fmt);
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
                    prec = skipAtoI(&fmt);
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
                    str = number(str, qlf == 'L' ? va_arg(args, unsigned long) : va_arg(args, unsigned int), 8, fld_w, prec, flags);
                    break;
                case 'p':
                    if (fld_w == -1) {
                        fld_w = 2 * sizeof(void *);
                        flags |= flag_fill_zero;
                    }
                    str = number(str, (unsigned long)va_arg(args, void *), 16, fld_w, prec, flags);
                    break;
                case 'x':
                case 'X':
                    str = number(str, qlf == 'L' ? va_arg(args, unsigned long) : va_arg(args, unsigned int), 16, fld_w, prec, flags);
                    break;
                case 'd':
                case 'i':
                    flags |= flag_sign;
                case 'u':
                    str = number(str, qlf == 'L' ? va_arg(args, unsigned long) : va_arg(args, unsigned int), 10, fld_w, prec, flags);
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

void scroll(void) {
    int x, y;
    unsigned int *addr = position.FBAddr, 
                *addr2 = position.FBAddr + position.YCharSize * position.XResolution;
    memcpy(addr2, addr, position.XResolution * (position.YResolution - position.YCharSize) * sizeof(u32));
    memset(addr + position.XResolution * (position.YResolution - position.YCharSize), 0, position.XResolution * position.YCharSize * sizeof(u32));
}

void drawchar(unsigned int fcol, unsigned int bcol, int px, int py, char ch) {
    int x, y;
    int testVal;
    unsigned int *addr;
    unsigned char *fontp = font_ascii[ch];
        for (y = 0; y < position.YCharSize; y++, fontp++) {
            addr = position.FBAddr + position.XResolution * (py + y) + px;
            testVal = 0x80;
            for (x = 0; x < position.XCharSize; x++, addr++, testVal >>= 1) {
                if (*fontp & testVal) *addr = fcol;
                else *addr = bcol;
            }
        }
}

inline void putchar(unsigned int fcol, unsigned int bcol, char ch) {
    int i;
    if (ch == '\n') {
        position.YPosition++, position.XPosition = 0;
        if (position.YPosition >= position.YResolution / position.YCharSize) {
            scroll();
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
        } while (position.XPosition >> 2 & 1);
    } else {
        if (position.XPosition == position.XResolution / position.XCharSize)
            putchar(fcol, bcol, '\n');
        drawchar(fcol, bcol, position.XCharSize * position.XPosition, position.YCharSize * position.YPosition, ch);
        position.XPosition++;
    }
}

void printk(unsigned int fcol, unsigned int bcol, const char *fmt, ...) {
    int len = 0, i;
    va_list args;
    va_start(args, fmt);
    len = sprintf(buf, fmt, args);
    va_end(args);
    for (i = 0; i < len; i++) 
        putchar(fcol, bcol, buf[i]);
}