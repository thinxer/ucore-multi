TARGET_ARCH := arm
TARGET_MACH :=

ifeq ($(TARGET_ARCH), arm)
	ifeq ($(TARGET_MACH),)
		TARGET_MACH := s3c2410
	endif
	TARGET_PREFIX:= arm-elf-
else ifeq ($(TARGET_MACH), x86)
	ifeq ($(TARGET_MACH),)
		TARGET_MACH := generic
	endif
	TARGET_PREFIX :=
endif
ARCH_DIR := arch/$(TARGET_ARCH)
MACH_DIR := arch/$(TARGET_ARCH)/$(TARGET_MACH)

ifeq (TARGET_ARCH, 'x86')
	SKYEYE := skyeye
	QEMU := qemu
else ifeq (TARGET_ARCH, 'arm')
	SKYEYE := skyeye
	QEMU := qemu-system-arm
endif

# for compiling tools to compile this package
HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

# for compiling to target arch
CC := $(TARGET_PREFIX)gcc
CFLAGS := -fno-builtin -Wall -ggdb -nostdinc -nostdlib
CFLAGS += -I . -I ./lib
CFLAGS += -I ./kern/driver -I ./kern/debug -I ./kern/trap
CFLAGS += -I $(ARCH_DIR) -I $(MACH_DIR)

LD			:= $(TARGET_PREFIX)ld
LDFALGS		:=

OBJCOPY		:= $(TARGET_PREFIX)objcopy
OBJDUMP		:= $(TARGET_PREFIX)objdump
GDB			:= $(TARGET_PREFIX)gdb

COPY		:= cp
MKDIR		:= mkdir -p
MV			:= mv
RM			:= rm -f
DIRNAME		:= dirname

.DEFAULT_GOAL	:= image

.PHONY: boot
boot: bin/boot.bin

bin/boot.bin: obj/boot/boot.o
	@$(MKDIR) `$(DIRNAME) $@`
	$(OBJCOPY) -O binary $< $@

obj/boot/boot.o: $(MACH_DIR)/boot/bootmain.c $(MACH_DIR)/boot/init.s
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -oobj/boot/bootmain.o $(MACH_DIR)/boot/bootmain.c
	$(CC) $(CFLAGS) -c -oobj/boot/init.o $(MACH_DIR)/boot/init.s
	$(CC) $(CFLAGS) $(LDFALGS) -T$(MACH_DIR)/boot/boot.lds -o $@ obj/boot/bootmain.o obj/boot/init.o

.PHONY: kernel
kernel: bin/kernel

KERN_OBJS 	:= obj/kern/init/init.o obj/kern/debug/panic.o
LIB_OBJS	:= obj/lib/printfmt.o obj/lib/string.o obj/lib/readline.o obj/lib/stdio.o
ARCH_OBJS 	:= obj/$(MACH_DIR)/clock.o obj/$(MACH_DIR)/console.o obj/$(MACH_DIR)/arch.o obj/$(MACH_DIR)/intr.o
ASM_OBJS	:= obj/$(MACH_DIR)/init.o obj/$(MACH_DIR)/div64.o obj/$(MACH_DIR)/intr_vector.o

$(ASM_OBJS):obj/%.o:%.S
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS):obj/%.o:%.c
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -o $@ $<

bin/kernel: $(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS) $(ASM_OBJS)
	@$(MKDIR) `$(DIRNAME) $@`
	$(LD) $(LDFALGS) -T $(MACH_DIR)/kernel.lds -o $@ $^

.PHONY: image
image: bin/ucore.img

bin/ucore.img: bin/boot.bin bin/kernel
	dd if=/dev/zero of=$@ bs=1MB count=8 2> /dev/null
	dd if=bin/boot.bin of=$@ conv=notrunc 2> /dev/null
	dd if=bin/kernel of=$@ bs=1K seek=64 conv=notrunc 2> /dev/null

.PHONY: skyeye
skyeye: image
	@cd tool/emu && skyeye -n

.PHONY: debug
debug: image
	@cd tool/emu && skyeye

.PHONY: gdb
gdb:
	$(GDB) -q -x tool/gdbinit

.PHONY: clean
clean:
	@$(RM) -r obj
	@$(RM) -r bin
