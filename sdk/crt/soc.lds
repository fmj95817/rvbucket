OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    DDR (rwx) : ORIGIN = 0x40000000, LENGTH = 0x200000
}

SECTIONS
{
    .text :
    {
        *(.init)
        *(.text.entry)
        *(.text .text.*)
    } > DDR

    .data : {
        *(.data .data.*)
        *(.rodata*)
    } > DDR

    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2)
        *(.srodata*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > DDR

    . = ALIGN(4);
    PROVIDE(__bss_start = .);
    .sbss (NOLOAD) : {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
    } > DDR
    .bss (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)
    } > DDR
    . = ALIGN(4);
    PROVIDE(__bss_end = .);

    .got : {
        *(.got .got.*)
    } > DDR

    /* End of uninitalized data segement */
    . = ALIGN(8);
    PROVIDE( _end = . );
    PROVIDE( end = . );

    PROVIDE(__stack_top = ORIGIN(DDR) + LENGTH(DDR));
    PROVIDE(__stack_size = 0x2000);

    .stack ORIGIN(DDR) + LENGTH(DDR) - __stack_size (NOLOAD) :
    {
        PROVIDE( _heap_end = . );
        . = __stack_size;
    } > DDR

    ASSERT(_end <= ADDR(.stack), "firmware exceeds DDR load window")
}
