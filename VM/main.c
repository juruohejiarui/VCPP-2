#include "vm.h"

int main(void) {
    printf("hello world\n");
    FILE *fPtr = fopen("./test.str", "rb");
    char *str = readString(fPtr);
    
    return 0;
}