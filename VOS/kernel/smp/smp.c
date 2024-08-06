#include "../includes/smp.h"
#include "../includes/hardware.h"
#include "../includes/log.h"

void SMP_init() {
    for (int i = 0; ; i++) {
        u32 a, b, c, d;
        HW_CPU_getID(0xb, i, &a, &b, &c, &d);
        if (((c >> 8) & 0xff) == 0) {
            printk(WHITE, BLACK, "SMP: x2 APIC: level:%d current logical processor:%d\n", c & 0xff, d);
            break;
        }
        printk(WHITE, BLACK, "SMP: local APIC: type:%d width:%d logical processor:%d\n",
            (c >> 8) & 0xff, a & 0x1f, b & 0xff);
    }
}