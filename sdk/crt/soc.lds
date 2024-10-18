OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    TCM (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x20000
}

SECTIONS
{
    . = 0x10000000;
    .text :
    {
        *(.init)
        *(.text.entry)
        *(.text)
    }

    /* data segment */
    .data : {
        *(.data .data.*)
    }

    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2)
        *(.srodata*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    }

    /* bss segment */
    .sbss : {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
    }
    .bss : { *(.bss .bss.*) }

    /* End of uninitalized data segement */
    . = ALIGN(8);
    PROVIDE( _end = . );
    PROVIDE( end = . );

    /* stack */
    PROVIDE(__stack_top = ORIGIN(TCM) + LENGTH(TCM));
	PROVIDE(__stack_size = 0x2000);

    .stack ORIGIN(TCM) + LENGTH(TCM) - __stack_size :
    {
        PROVIDE( _heap_end = . );
        . = __stack_size;
    }
}
