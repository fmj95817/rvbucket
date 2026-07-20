OUTPUT_ARCH("riscv")
ENTRY(_boot)

MEMORY
{
    BOOT_ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x1000
}

SECTIONS
{
    .text 0x00000000 : {
        KEEP(*(.text.boot))
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > BOOT_ROM
}
