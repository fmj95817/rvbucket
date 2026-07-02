OUTPUT_ARCH("riscv")
ENTRY(_boot)

MEMORY
{
    BOOT_ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x800
}

SECTIONS
{
    .text : {
        *(*)
    } > BOOT_ROM
}
