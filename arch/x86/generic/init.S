/*
kernel init asm file.
*/
.global _kern_init

_kern_init:
	call kern_init
