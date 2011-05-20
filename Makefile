# target architecture
TARGET_ARCH := x86
# target machine
TARGET_MACH :=

# set target prefix and default machine type
ifeq ($(TARGET_ARCH), arm)
	ifeq ($(TARGET_MACH),)
		TARGET_MACH := s3c2410
	endif
	TARGET_PREFIX:= arm-elf-
else ifeq ($(TARGET_ARCH), x86)
	ifeq ($(TARGET_MACH),)
		TARGET_MACH := generic
	endif
	TARGET_PREFIX :=
endif

# architecture directory
ARCH_DIR := arch/$(TARGET_ARCH)
# machine specific directory
MACH_DIR := arch/$(TARGET_ARCH)/$(TARGET_MACH)

# set emulators
ifeq ($(TARGET_ARCH), x86)
	SKYEYE := skyeye
	QEMU := qemu
else ifeq ($(TARGET_ARCH), arm)
	SKYEYE := skyeye
	QEMU := qemu-system-arm
endif

# for compiling tools to compile this package
HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

# for compiling to target arch
ifeq ($(TARGET_ARCH), x86)
CC := $(TARGET_PREFIX)gcc-4.4
else ifeq ($(TARGET_ARCH), arm)
CC := $(TARGET_PREFIX)gcc
endif
# general flags
CFLAGS := -fno-builtin -Wall -nostdinc -nostdlib -fno-stack-protector -gstabs -O2
# includes
CFLAGS += -I ./include -I ./kern/include

# addition flags depending on target_arch
ifeq ($(TARGET_ARCH), x86)
	# force 32 bit
	CFLAGS += -m32
endif

# linker
LD			:= $(TARGET_PREFIX)ld
LDFLAGS		:= -nostdlib -N

ifeq ($(TARGET_ARCH), x86)
	LDFLAGS += -m elf_i386
endif

# general commands
OBJCOPY		:= $(TARGET_PREFIX)objcopy
OBJDUMP		:= $(TARGET_PREFIX)objdump
GDB			:= $(TARGET_PREFIX)gdb

COPY		:= cp
MKDIR		:= mkdir -p
MV			:= mv
RM			:= rm -f
DIRNAME		:= dirname

.DEFAULT_GOAL := image

# link include directories
.PHONY: prepare
prepare:
	cd include && ln -s ../$(ARCH_DIR)/include arch
	cd include && ln -s ../$(MACH_DIR)/include mach


.PHONY: boot
boot: bin/boot.bin

bin/boot.bin: obj/boot/boot.o
	@$(MKDIR) `$(DIRNAME) $@`
	$(OBJCOPY) -S -O binary $< $@

obj/boot/boot.o: $(MACH_DIR)/boot/bootasm.S $(MACH_DIR)/boot/bootmain.c
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -oobj/boot/bootmain.o $(MACH_DIR)/boot/bootmain.c
	$(CC) $(CFLAGS) -c -oobj/boot/bootasm.o $(MACH_DIR)/boot/bootasm.S
	$(LD) $(LDFLAGS) -T$(MACH_DIR)/boot/boot.lds -o $@ obj/boot/bootasm.o obj/boot/bootmain.o

.PHONY: kernel
kernel: bin/kernel

KERN_OBJS 	:=	obj/kern/init/init.o \
				obj/kern/debug/panic.o \
				# obj/kern/mm/buddy_pmm.o
LIB_OBJS	:=	obj/lib/printfmt.o\
				obj/lib/string.o\
				obj/lib/readline.o\
				obj/lib/stdio.o
ARCH_OBJS 	:=	obj/$(MACH_DIR)/clock.o \
				obj/$(MACH_DIR)/console.o \
				obj/$(MACH_DIR)/intr.o \
				# obj/$(MACH_DIR)/pmm.o
ASM_OBJS	:=	obj/$(MACH_DIR)/init.o\
				obj/$(MACH_DIR)/intr_vector.o

ifeq ($(TARGET_ARCH), arm)
ARCH_OBJS	+=	obj/$(ARCH_DIR)/lib/div0.o
ASM_OBJS	+=	obj/$(ARCH_DIR)/lib/_umodsi3.o \
				obj/$(ARCH_DIR)/lib/_modsi3.o \
				obj/$(ARCH_DIR)/lib/_udivsi3.o \
				obj/$(ARCH_DIR)/lib/_divsi3.o \
				obj/$(ARCH_DIR)/lib/div64.o
endif

$(ASM_OBJS):obj/%.o:%.S
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS):obj/%.o:%.c
	@$(MKDIR) `$(DIRNAME) $@`
	$(CC) $(CFLAGS) -c -o $@ $<

bin/kernel: $(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS) $(ASM_OBJS)
	@$(MKDIR) `$(DIRNAME) $@`
	$(LD) $(LDFLAGS) -T $(MACH_DIR)/kernel.lds -o $@ $^

.PHONY: image
image: bin/ucore.img

ifeq ($(TARGET_ARCH), x86)
# x86 needs the sign tool
sign: bin/sign

bin/sign: $(ARCH_DIR)/tool/sign.c
	$(HOSTCC) $(HOSTCFLAGS) -o$@ $^

bin/ucore.img: bin/boot.bin bin/sign bin/kernel
	bin/sign bin/boot.bin bin/boot.bin.signed
	dd if=/dev/zero of=$@ bs=1MB count=8 2> /dev/null
	dd if=bin/boot.bin.signed of=$@ conv=notrunc 2> /dev/null
	dd if=bin/kernel of=$@ seek=1 conv=notrunc 2> /dev/null
else
bin/ucore.img: bin/boot.bin bin/kernel
	dd if=/dev/zero of=$@ bs=1MB count=8 2> /dev/null
	dd if=bin/boot.bin of=$@ conv=notrunc 2> /dev/null
	dd if=bin/kernel of=$@ bs=1K seek=64 conv=notrunc 2> /dev/null
endif

.PHONY: skyeye debug-skyeye
skyeye: image
	@cd tool/emu && skyeye -n

dskyeye: image
	@cd tool/emu && skyeye

.PHONY: qemu debug-qemu
qemu: image
	$(QEMU) -parallel stdio -serial null -hda bin/ucore.img
dqemu: image
	$(QEMU) -parallel stdio -serial null -S -s -hda bin/ucore.img -gdb tcp::12345

.PHONY: gdb
gdb:
	$(GDB) -q -x tool/gdbinit

.PHONY: clean
clean:
	@$(RM) -r obj
	@$(RM) -r bin
	@$(RM) include/arch
	@$(RM) include/mach

#  vim: set noet ts=4 sw=4 tw=0 :
