#include "../includes/interrupt.h"
#include "../includes/lib.h"
#include "../includes/linkage.h"

void Init_8259A() {
    IO_out8(0x20, 0x11);
    IO_out8(0x21, 0x20);
    IO_out8(0x21, 0x04);
    IO_out8(0x21, 0x01);

    IO_out8(0xa0, 0x11);
    IO_out8(0xa1, 0x28);
    IO_out8(0xa1, 0x02);
    IO_out8(0xa1, 0x01);

    IO_out8(0x21, 0xfd);
    IO_out8(0xa1, 0xff);

    IO_sti();
}