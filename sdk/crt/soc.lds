OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    ITCM (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x20000
    DTCM (rw)   : ORIGIN = 0x20000000, LENGTH = 0x10000
}

SECTIONS
{
    .text :
    {
        *(.init)
        *(.text.entry)
        *(.text)
    } > ITCM

    /* data segment */
    .data : {
        *(.data .data.*)
        *(.rodata*)
    } > DTCM

    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2)
        *(.srodata*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > DTCM

    /* bss segment */
    .sbss : {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
    } > DTCM
    .bss : {
        *(.bss .bss.*)
    } > DTCM

    .got : {
        *(.got .got.*)
    } > DTCM

    /* End of uninitalized data segement */
    . = ALIGN(8);
    PROVIDE( _end = . );
    PROVIDE( end = . );

    /* stack */
    PROVIDE(__stack_top = ORIGIN(DTCM) + LENGTH(DTCM));
	PROVIDE(__stack_size = 0x2000);

    .stack ORIGIN(DTCM) + LENGTH(DTCM) - __stack_size :
    {
        PROVIDE( _heap_end = . );
        . = __stack_size;
    } > DTCM
}
