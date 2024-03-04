ASM = as
CC = gcc-4.7

ASMFLAG = --64
CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -m64

ASMOBJS = head.o entry.o
SRCS = $(wildcard ./*.c) $(wildcard interrupt/*.c) $(wildcard log/*.c) $(wildcard includes/*.c)
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

head.o: head.S
	$(CXX) -E head.S > head.s
	$(ASM) $(ASMFLAG) -o head.o head.s
entry.o: entry.S
	$(CXX) -E entry.S > entry.s
	$(ASM) $(ASMFLAG) -o entry.o entry.s

clear:
	rm -R ./*.s $(ALLOBJS) system_tmp system kernel.bin