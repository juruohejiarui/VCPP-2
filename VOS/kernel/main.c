#include "./includes/printk.h"
#include "./includes/trap.h"
#include "./includes/gate.h"

void Start_Kernel(void) {
    char ch = '\0', i;
    unsigned int fcol = 0x00ffffff, bcol = 0x000000;
    position.XResolution = 1440;
    position.YResolution = 900;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;

    
    loadTR(8);
    setTSS64(   0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
                0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
    systemVectorInit();
    // i = 1 / 0;
    // i = *(unsigned char *)(0xffff80000aa00000);
    while (1) ;
}