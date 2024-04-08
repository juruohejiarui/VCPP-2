ASM = as
CC = gcc-4.7

ASMFLAG = --64
CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -m64 -std=c99

ASMOBJS = head.o entry.o pgtbl.o
SRCS = $(wildcard ./*.c) \
	$(wildcard interrupt/*.c) \
	$(wildcard log/*.c) \
	$(wildcard includes/*.c) \
	$(wildcard memory/*.c) \
	$(wildcard lib/*.c) \
	$(wildcard task/*.c) \
	$(wildcard hardware/*.c)
CXXOBJS = $(SRCS:.c=.o)

ALLOBJS = $(ASMOBJS) $(CXXOBJS) 

all: system_tmp $(ALLOBJS)
	ld -b elf64-x86-64 -z muldefs -o system $(ALLOBJS) -T Kernel.lds 
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
system_tmp: $(ALLOBJS)
	ld -b elf64-x86-64 -z muldefs -o system_tmp $(ALLOBJS) -T Kernel.lds 

.depend: $(SRCS)
	rm -f .depend
	$(foreach src, $(SRCS), \
		echo -n $(dir $(src)) >> .depend; \
		gcc -I. -MM $(src) >> .depend; \
	)
include .depend

pgtbl.o: memory/pgtbl.S includes/lib.h includes/memory.h includes/linkage.h
	$(CC) -E memory/pgtbl.S > pgtbl.s
	$(ASM) $(ASMFLAG) -o pgtbl.o pgtbl.s
head.o: head.S
	$(CC) -E head.S > head.s
	$(ASM) $(ASMFLAG) -o head.o head.s
entry.o: ./interrupt/entry.S
	$(CC) -E ./interrupt/entry.S > entry.s
	$(ASM) $(ASMFLAG) -o entry.o entry.s

clear:
	rm -R ./*.s $(ALLOBJS) system_tmp system kernel.bin