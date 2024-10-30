OUTPUT_ARCH("riscv")
ENTRY(_boot)

MEMORY
{
    BOOT_ROM (rx)  : ORIGIN = 0x40000000, LENGTH = 0x4000
}

SECTIONS
{
    .text :
    {
        *(.text .text.*)
    } > BOOT_ROM
}
