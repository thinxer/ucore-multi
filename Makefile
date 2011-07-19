# target architecture
TARGET_ARCH := arm
# target machine
TARGET_MACH := red2

TARGET_PREFIX:= arm-elf-

# architecture directory
ARCH_DIR := arch/$(TARGET_ARCH)
# machine specific directory
MACH_DIR := arch/$(TARGET_ARCH)/$(TARGET_MACH)

# set emulators
SKYEYE := skyeye
QEMU := qemu-system-arm

# for compiling tools to compile this package
HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

# for compiling to target arch
CC := $(TARGET_PREFIX)gcc
# general flags
CFLAGS := -fno-builtin -Wall -nostdinc -nostdlib -fno-stack-protector -gstabs -O2
# includes
CFLAGS += -I ./include -I ./kern/include

# linker
LD			:= $(TARGET_PREFIX)ld
LDFLAGS		:= -nostdlib -N

# general commands
OBJCOPY		:= $(TARGET_PREFIX)objcopy
OBJDUMP		:= $(TARGET_PREFIX)objdump
GDB			:= $(TARGET_PREFIX)gdb

COPY		:= cp
MKDIR		:= mkdir -p
MV			:= mv
RM			:= rm -f
DIRNAME		:= dirname

.DEFAULT_GOAL := kernel

# link include directories
.PHONY: prepare
prepare: clean
	@cd include && ln -s ../$(ARCH_DIR)/include arch
	@cd include && ln -s ../$(MACH_DIR)/include mach


.PHONY: kernel
kernel: bin/kernel

KERN_OBJS 	:=	obj/kern/init/init.o\
				obj/kern/debug/panic.o\
				obj/kern/mm/buddy_pmm.o\
				obj/kern/mm/slab.o\
				obj/kern/mm/vmm.o\
				obj/kern/mm/shmem.o
LIB_OBJS	:=	obj/lib/printfmt.o\
				obj/lib/string.o\
				obj/lib/readline.o\
				obj/lib/stdio.o\
				obj/lib/rand.o\
				obj/lib/rb_tree.o
ARCH_OBJS 	:=	obj/$(MACH_DIR)/clock.o\
				obj/$(MACH_DIR)/console.o \
				obj/$(MACH_DIR)/intr.o \
				obj/$(ARCH_DIR)/pmm.o
ASM_OBJS	:=	obj/$(MACH_DIR)/init.o\
				obj/$(MACH_DIR)/intr_vector.o

ifeq ($(TARGET_ARCH), arm)
ARCH_OBJS	+=	obj/$(ARCH_DIR)/lib/div0.o\
				obj/$(MACH_DIR)/memmap.o
ASM_OBJS	+=	obj/$(ARCH_DIR)/lib/_umodsi3.o \
				obj/$(ARCH_DIR)/lib/_modsi3.o \
				obj/$(ARCH_DIR)/lib/_udivsi3.o \
				obj/$(ARCH_DIR)/lib/_divsi3.o \
				obj/$(ARCH_DIR)/lib/div64.o
endif

$(ASM_OBJS):obj/%.o:%.S
	@$(MKDIR) `$(DIRNAME) $@`
	@echo cc $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS):obj/%.o:%.c
	@$(MKDIR) `$(DIRNAME) $@`
	@echo cc $<
	@$(CC) $(CFLAGS) -c -o $@ $<

bin/kernel: $(KERN_OBJS) $(LIB_OBJS) $(ARCH_OBJS) $(ASM_OBJS)
	@$(MKDIR) `$(DIRNAME) $@`
	@echo cc $<
	@$(LD) $(LDFLAGS) -T $(MACH_DIR)/kernel.lds -o $@ $^

.PHONY: skyeye debug-skyeye
skyeye: kernel
	@cd tool/emu && skyeye -n

dskyeye: kernel
	@cd tool/emu && skyeye

.PHONY: qemu debug-qemu
qemu: kernel
	$(QEMU) -parallel stdio -serial null -hda bin/ucore.img
dqemu: kernel
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
