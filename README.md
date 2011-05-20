#description

This is an effort to port ucore to ARM, and keep the current x86 version work.

'ucore' is a teaching operating system used by Tsinghua University.
Project homepage: [http://ucore.googlecode.com].

Currently the ARM port runs on the S3C2410X microprocessor, which is developed
using an ARM920T core.

#running

You need gcc 4.4 or 4.5 to compile for the x86 architecture. gcc 4.6 is known
not to work.

    make TARGET_ARCH=x86 prepare
    make TARGET_ARCH=x86 image

Compile for the ARM architecture:

    make TARGET_ARCH=arm prepare
    make TARGET_ARCH=arm image

#source code organization

* 'arch' contains architecture dependent driver and boot code.
* 'lib' contains architecture independent library code.
* 'kern' contains architecture independent kernel code.
* 'tool' contains various tools such as GDB scripts and emulator configurations.

#current status

* Simple bootloader works, which can load elf kernel from memory.
* Output to UART works. cprintf works.
* Interrupt(IRQ) works.

#todo

* Implement memory management system.
* Implement syscall.
* Input from UART.
* Rewrite Makefile, make it DRY and more like original ucore Makefile.

#acknowledgment

* Most code is taken from the ucore project, of course.
* Boot and interrupt code is inspired by david leels <davidontech@gmail.com>.
* div64 code for arm is taken from the Linux kernel.
