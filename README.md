#description

This is an effort to port ucore to ARM, and keep the current x86 version work.

'ucore' is a teaching operating system used by Tsinghua University.
Project homepage: [http://ucore.googlecode.com].

Currently the ARM port runs on the S3C2410X microprocessor, which is developed
using an ARM920T core.

#running

A recent version of gcc should work just well. My gcc version is 4.6.0.

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
* PMM works.
* Slab works.

#todo

* Move kernel base to 0xC000000 from 0x3000000.
* Implement mmap.
* Draw a map of memlayout of ARM.
* Clear system stack address.
* Implement syscall.
* Input from UART.
* Swap?
* Processes.

#acknowledgment

Hail all open source software developers.

* Most code is taken from the ucore project, of course.
* Boot and interrupt code is inspired by david leels <davidontech@gmail.com>.
* div64 code for arm is taken from the Linux kernel.
* Software div/mode code is taken from libgcc.
