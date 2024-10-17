#include "soc.h"

int main()
{
    soc_t soc;

    soc_construct(&soc);
    rom_burn(&soc.flash, NULL, 0, 0);
    soc_reset(&soc);

    for (int i = 0; i < 100; i++) {
        rv32i_exec(&soc.cpu);
    }

    soc_free(&soc);
    return 0;
}