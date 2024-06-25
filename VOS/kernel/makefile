ASM = as
CC = gcc-4.7

ASMFLAG = --64
CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -Wno-packed-bitfield-compat -mno-red-zone -m64 -std=gnu89 -std=c99

.DEFAULT_GOAL = all

include ./.depend

all: $(ALLOBJS)
	ld -b elf64-x86-64 -z muldefs -o system $(ALLOBJS) -T Kernel.lds 
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
# system_tmp: $(ALLOBJS)
#	ld -b elf64-x86-64 -z muldefs -o system_tmp $(ALLOBJS) -T Kernel.lds 

clear:
	rm -R ./*.s ./*/*.s $(ALLOBJS) system kernel.bin