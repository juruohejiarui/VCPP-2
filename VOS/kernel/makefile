ASM = as
CC = gcc

ASMFLAG = --64
CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -Wno-packed-bitfield-compat -Wno-address-of-packed-member -mno-red-zone -m64 -std=gnu89 -std=c99

.DEFAULT_GOAL = all

include ./.depend

all: system_tmp ../Tools/kallsyms.o
	@ld -b elf64-x86-64 -z muldefs -o system $(ALLOBJS) ../Tools/kallsyms.o -T Kernel.lds --no-warn-rwx-segments
	@echo "[LD] system"
	@objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
	@echo "[OBJDUMP] kernel.bin"

system_tmp: $(ALLOBJS)
	@ld -b elf64-x86-64 -z muldefs -o system_tmp $(ALLOBJS) -T Kernel.lds --no-warn-rwx-segments
	@echo "[LD] system_tmp"

../Tools/kallsyms.o: ../Tools/kallsyms.c system_tmp
	@$(CC) -o ../Tools/kallsyms ../Tools/kallsyms.c
	@nm -n system_tmp | ../Tools/kallsyms > ../Tools/kallsyms.S
	@$(CC) -c ../Tools/kallsyms.S -o ../Tools/kallsyms.o -Wl,--no-warn-rwx-segments
	@echo "[Multi] kallsyms.o"

clean:
	rm -R system system_tmp kernel.bin .depend ../Tools/kallsyms.S ../Tools/kallsyms ../Tools/kallsyms.o
	@python3 clear.py
	@echo "[SCRIPT] clear.py"