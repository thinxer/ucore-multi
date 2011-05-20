OUTPUT_ARCH(i386)
ENTRY(start)

SECTIONS
{
    . = 0x7C00;
    .text :
    {
        *(.text)
    }
    . = ALIGN(32);

    .data :
    {
        *(.data)
    }

    . = ALIGN(32);
    __bss_start__ = .;
    .bss :
    {
        *(.bss)
    }
    __bss_end__ = .;

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
