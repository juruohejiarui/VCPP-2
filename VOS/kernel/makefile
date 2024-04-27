PIC = APIC
ASM = as
CC = gcc-4.7

ASMFLAG = --64
CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -m64 -std=gnu89 -std=c99 -D$(PIC)

CSRCS = $(wildcard ./*.c) \
	$(wildcard interrupt/*.c) \
	$(wildcard log/*.c) \
	$(wildcard includes/*.c) \
	$(wildcard memory/*.c) \
	$(wildcard lib/*.c) \
	$(wildcard task/*.c) \
	$(wildcard hardware/*.c)
CXXOBJS = $(CSRCS:.c=.o)
ASMSRCS = $(wildcard ./*.S) \
	$(wildcard interrupt/*.S) \
	$(wildcard log/*.S) \
	$(wildcard includes/*.S) \
	$(wildcard memory/*.S) \
	$(wildcard lib/*.S) \
	$(wildcard task/*.S) \
	$(wildcard hardware/*.S)
ASMOBJS = $(ASMSRCS:.S=.o)

ALLOBJS = $(ASMOBJS) $(CXXOBJS) 

all: system_tmp $(ALLOBJS)
	ld -b elf64-x86-64 -z muldefs -o system $(ALLOBJS) -T Kernel.lds 
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
system_tmp: $(ALLOBJS)
	ld -b elf64-x86-64 -z muldefs -o system_tmp $(ALLOBJS) -T Kernel.lds 

.depend: $(CSRCS)
	rm -f .depend
	$(foreach src, $(SRCS), \
		echo -n $(dir $(src)) >> .depend; \
		gcc -I. -MM $(src) >> .depend; \
	)
include .depend
head.o: head.S
	$(CC) -E head.S > head.s
	$(ASM) $(ASMFLAG) -o head.o head.s
interrupt/entry.o: interrupt/entry.S
	$(CC) -E interrupt/entry.S > interrupt/entry.s
	$(ASM) $(ASMFLAG) -o interrupt/entry.o interrupt/entry.s
memory/croper.o: memory/croper.S
	$(CC) -E memory/croper.S > memory/croper.s
	$(ASM) $(ASMFLAG) -o memory/croper.o memory/croper.s

clear:
	rm -R ./*.s ./*/*.s $(ALLOBJS) system_tmp system kernel.bin