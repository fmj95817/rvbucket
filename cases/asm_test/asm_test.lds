OUTPUT_ARCH("riscv")
ENTRY(_asm_test)

MEMORY
{
    BOOT_ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x4000
}

SECTIONS
{
    .text :
    {
        *(.text .text.*)
    } > BOOT_ROM
}
