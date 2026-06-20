#include "csr.h"

#define CSR_CHECK_PRIV(p) if (priv < p) { return false; }

void rv32g_csr_reset(rv32g_csr_t *csr)
{
    csr->sstatus.raw = 0;
    csr->sie.raw = 0;
    csr->stvec.raw = 0;
    csr->scounteren.raw = 0;
    csr->senvcfg.raw = 0;
    csr->sscratch = 0;
    csr->sepc = 0;
    csr->scause.raw = 0;
    csr->stval = 0;
    csr->sip.raw = 0;
    csr->stimecmp = 0;
    csr->stimecmph = 0;
    csr->satp.raw = 0;
    csr->mstatus.raw = 0;
    csr->misa.raw = RV32G_CSR_MISA_I_BIT | RV32G_CSR_MISA_M_BIT | RV32G_CSR_MISA_A_BIT | RV32G_CSR_MISA_U_BIT | RV32G_CSR_MISA_S_BIT | (1u << 30);
    csr->medeleg.raw = 0;
    csr->mideleg.raw = 0;
    csr->mie.raw = 0;
    csr->mtvec.raw = 0;
    csr->mcounteren.raw = 0;
    csr->menvcfg.raw = 0;
    csr->mstatush.raw = 0;
    csr->menvcfgh.raw = 0;
    csr->mcountinhibit.raw = 0;
    csr->mcyclecfg = 0;
    csr->minstretcfg = 0;
    csr->mhpmevent3 = 0;
    csr->mhpmevent4 = 0;
    csr->mhpmevent5 = 0;
    csr->mhpmevent6 = 0;
    csr->mhpmevent7 = 0;
    csr->mhpmevent8 = 0;
    csr->mhpmevent9 = 0;
    csr->mhpmevent10 = 0;
    csr->mhpmevent11 = 0;
    csr->mhpmevent12 = 0;
    csr->mhpmevent13 = 0;
    csr->mhpmevent14 = 0;
    csr->mhpmevent15 = 0;
    csr->mhpmevent16 = 0;
    csr->mhpmevent17 = 0;
    csr->mhpmevent18 = 0;
    csr->mhpmevent19 = 0;
    csr->mhpmevent20 = 0;
    csr->mhpmevent21 = 0;
    csr->mhpmevent22 = 0;
    csr->mhpmevent23 = 0;
    csr->mhpmevent24 = 0;
    csr->mhpmevent25 = 0;
    csr->mhpmevent26 = 0;
    csr->mhpmevent27 = 0;
    csr->mhpmevent28 = 0;
    csr->mhpmevent29 = 0;
    csr->mhpmevent30 = 0;
    csr->mhpmevent31 = 0;
    csr->mcyclecfgh = 0;
    csr->minstretcfgh = 0;
    csr->mhpmevent3h = 0;
    csr->mhpmevent4h = 0;
    csr->mhpmevent5h = 0;
    csr->mhpmevent6h = 0;
    csr->mhpmevent7h = 0;
    csr->mhpmevent8h = 0;
    csr->mhpmevent9h = 0;
    csr->mhpmevent10h = 0;
    csr->mhpmevent11h = 0;
    csr->mhpmevent12h = 0;
    csr->mhpmevent13h = 0;
    csr->mhpmevent14h = 0;
    csr->mhpmevent15h = 0;
    csr->mhpmevent16h = 0;
    csr->mhpmevent17h = 0;
    csr->mhpmevent18h = 0;
    csr->mhpmevent19h = 0;
    csr->mhpmevent20h = 0;
    csr->mhpmevent21h = 0;
    csr->mhpmevent22h = 0;
    csr->mhpmevent23h = 0;
    csr->mhpmevent24h = 0;
    csr->mhpmevent25h = 0;
    csr->mhpmevent26h = 0;
    csr->mhpmevent27h = 0;
    csr->mhpmevent28h = 0;
    csr->mhpmevent29h = 0;
    csr->mhpmevent30h = 0;
    csr->mhpmevent31h = 0;
    csr->mscratch = 0;
    csr->mepc = 0;
    csr->mcause.raw = 0;
    csr->mtval = 0;
    csr->mip.raw = 0;
    csr->mtinst = 0;
    csr->mtval2 = 0;
    csr->pmpcfg0 = 0;
    csr->pmpcfg1 = 0;
    csr->pmpcfg2 = 0;
    csr->pmpcfg3 = 0;
    csr->pmpcfg4 = 0;
    csr->pmpcfg5 = 0;
    csr->pmpcfg6 = 0;
    csr->pmpcfg7 = 0;
    csr->pmpcfg8 = 0;
    csr->pmpcfg9 = 0;
    csr->pmpcfg10 = 0;
    csr->pmpcfg11 = 0;
    csr->pmpcfg12 = 0;
    csr->pmpcfg13 = 0;
    csr->pmpcfg14 = 0;
    csr->pmpcfg15 = 0;
    csr->pmpaddr0 = 0;
    csr->pmpaddr1 = 0;
    csr->pmpaddr2 = 0;
    csr->pmpaddr3 = 0;
    csr->pmpaddr4 = 0;
    csr->pmpaddr5 = 0;
    csr->pmpaddr6 = 0;
    csr->pmpaddr7 = 0;
    csr->pmpaddr8 = 0;
    csr->pmpaddr9 = 0;
    csr->pmpaddr10 = 0;
    csr->pmpaddr11 = 0;
    csr->pmpaddr12 = 0;
    csr->pmpaddr13 = 0;
    csr->pmpaddr14 = 0;
    csr->pmpaddr15 = 0;
    csr->pmpaddr16 = 0;
    csr->pmpaddr17 = 0;
    csr->pmpaddr18 = 0;
    csr->pmpaddr19 = 0;
    csr->pmpaddr20 = 0;
    csr->pmpaddr21 = 0;
    csr->pmpaddr22 = 0;
    csr->pmpaddr23 = 0;
    csr->pmpaddr24 = 0;
    csr->pmpaddr25 = 0;
    csr->pmpaddr26 = 0;
    csr->pmpaddr27 = 0;
    csr->pmpaddr28 = 0;
    csr->pmpaddr29 = 0;
    csr->pmpaddr30 = 0;
    csr->pmpaddr31 = 0;
    csr->pmpaddr32 = 0;
    csr->pmpaddr33 = 0;
    csr->pmpaddr34 = 0;
    csr->pmpaddr35 = 0;
    csr->pmpaddr36 = 0;
    csr->pmpaddr37 = 0;
    csr->pmpaddr38 = 0;
    csr->pmpaddr39 = 0;
    csr->pmpaddr40 = 0;
    csr->pmpaddr41 = 0;
    csr->pmpaddr42 = 0;
    csr->pmpaddr43 = 0;
    csr->pmpaddr44 = 0;
    csr->pmpaddr45 = 0;
    csr->pmpaddr46 = 0;
    csr->pmpaddr47 = 0;
    csr->pmpaddr48 = 0;
    csr->pmpaddr49 = 0;
    csr->pmpaddr50 = 0;
    csr->pmpaddr51 = 0;
    csr->pmpaddr52 = 0;
    csr->pmpaddr53 = 0;
    csr->pmpaddr54 = 0;
    csr->pmpaddr55 = 0;
    csr->pmpaddr56 = 0;
    csr->pmpaddr57 = 0;
    csr->pmpaddr58 = 0;
    csr->pmpaddr59 = 0;
    csr->pmpaddr60 = 0;
    csr->pmpaddr61 = 0;
    csr->pmpaddr62 = 0;
    csr->pmpaddr63 = 0;
    csr->tselect = 0;
    csr->tinfo = 0;
    csr->mhpmcounter3 = 0;
    csr->mhpmcounter4 = 0;
    csr->mhpmcounter5 = 0;
    csr->mhpmcounter6 = 0;
    csr->mhpmcounter7 = 0;
    csr->mhpmcounter8 = 0;
    csr->mhpmcounter9 = 0;
    csr->mhpmcounter10 = 0;
    csr->mhpmcounter11 = 0;
    csr->mhpmcounter12 = 0;
    csr->mhpmcounter13 = 0;
    csr->mhpmcounter14 = 0;
    csr->mhpmcounter15 = 0;
    csr->mhpmcounter16 = 0;
    csr->mhpmcounter17 = 0;
    csr->mhpmcounter18 = 0;
    csr->mhpmcounter19 = 0;
    csr->mhpmcounter20 = 0;
    csr->mhpmcounter21 = 0;
    csr->mhpmcounter22 = 0;
    csr->mhpmcounter23 = 0;
    csr->mhpmcounter24 = 0;
    csr->mhpmcounter25 = 0;
    csr->mhpmcounter26 = 0;
    csr->mhpmcounter27 = 0;
    csr->mhpmcounter28 = 0;
    csr->mhpmcounter29 = 0;
    csr->mhpmcounter30 = 0;
    csr->mhpmcounter31 = 0;
    csr->mhpmcounter3h = 0;
    csr->mhpmcounter4h = 0;
    csr->mhpmcounter5h = 0;
    csr->mhpmcounter6h = 0;
    csr->mhpmcounter7h = 0;
    csr->mhpmcounter8h = 0;
    csr->mhpmcounter9h = 0;
    csr->mhpmcounter10h = 0;
    csr->mhpmcounter11h = 0;
    csr->mhpmcounter12h = 0;
    csr->mhpmcounter13h = 0;
    csr->mhpmcounter14h = 0;
    csr->mhpmcounter15h = 0;
    csr->mhpmcounter16h = 0;
    csr->mhpmcounter17h = 0;
    csr->mhpmcounter18h = 0;
    csr->mhpmcounter19h = 0;
    csr->mhpmcounter20h = 0;
    csr->mhpmcounter21h = 0;
    csr->mhpmcounter22h = 0;
    csr->mhpmcounter23h = 0;
    csr->mhpmcounter24h = 0;
    csr->mhpmcounter25h = 0;
    csr->mhpmcounter26h = 0;
    csr->mhpmcounter27h = 0;
    csr->mhpmcounter28h = 0;
    csr->mhpmcounter29h = 0;
    csr->mhpmcounter30h = 0;
    csr->mhpmcounter31h = 0;
    csr->cycle = 0;
    csr->time = 0;
    csr->instret = 0;
    csr->cycleh = 0;
    csr->timeh = 0;
    csr->scountovf = 0;
    csr->mvendorid = 0;
    csr->marchid = 0;
    csr->mimpid = 0;
    csr->mhartid = 0;
    csr->mtopi = 0;
}

bool rv32g_csr_read(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data)
{
    switch (addr) {
        case 0x100: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sstatus.raw;
            return true;
        }
        case 0x104: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sie.raw;
            return true;
        }
        case 0x105: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->stvec.raw;
            return true;
        }
        case 0x106: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->scounteren.raw;
            return true;
        }
        case 0x10a: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->senvcfg.raw;
            return true;
        }
        case 0x140: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sscratch;
            return true;
        }
        case 0x141: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sepc;
            return true;
        }
        case 0x142: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->scause.raw;
            return true;
        }
        case 0x143: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->stval;
            return true;
        }
        case 0x144: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sip.raw;
            return true;
        }
        case 0x14d: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->stimecmp;
            return true;
        }
        case 0x15d: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->stimecmph;
            return true;
        }
        case 0x180: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->satp.raw;
            return true;
        }
        case 0x300: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mstatus.raw;
            return true;
        }
        case 0x301: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->misa.raw;
            return true;
        }
        case 0x302: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->medeleg.raw;
            return true;
        }
        case 0x303: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mideleg.raw;
            return true;
        }
        case 0x304: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mie.raw;
            return true;
        }
        case 0x305: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mtvec.raw;
            return true;
        }
        case 0x306: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mcounteren.raw;
            return true;
        }
        case 0x30a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->menvcfg.raw;
            return true;
        }
        case 0x310: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mstatush.raw;
            return true;
        }
        case 0x31a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->menvcfgh.raw;
            return true;
        }
        case 0x320: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mcountinhibit.raw;
            return true;
        }
        case 0x321: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mcyclecfg;
            return true;
        }
        case 0x322: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->minstretcfg;
            return true;
        }
        case 0x323: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent3;
            return true;
        }
        case 0x324: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent4;
            return true;
        }
        case 0x325: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent5;
            return true;
        }
        case 0x326: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent6;
            return true;
        }
        case 0x327: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent7;
            return true;
        }
        case 0x328: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent8;
            return true;
        }
        case 0x329: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent9;
            return true;
        }
        case 0x32a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent10;
            return true;
        }
        case 0x32b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent11;
            return true;
        }
        case 0x32c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent12;
            return true;
        }
        case 0x32d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent13;
            return true;
        }
        case 0x32e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent14;
            return true;
        }
        case 0x32f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent15;
            return true;
        }
        case 0x330: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent16;
            return true;
        }
        case 0x331: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent17;
            return true;
        }
        case 0x332: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent18;
            return true;
        }
        case 0x333: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent19;
            return true;
        }
        case 0x334: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent20;
            return true;
        }
        case 0x335: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent21;
            return true;
        }
        case 0x336: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent22;
            return true;
        }
        case 0x337: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent23;
            return true;
        }
        case 0x338: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent24;
            return true;
        }
        case 0x339: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent25;
            return true;
        }
        case 0x33a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent26;
            return true;
        }
        case 0x33b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent27;
            return true;
        }
        case 0x33c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent28;
            return true;
        }
        case 0x33d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent29;
            return true;
        }
        case 0x33e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent30;
            return true;
        }
        case 0x33f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent31;
            return true;
        }
        case 0x721: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mcyclecfgh;
            return true;
        }
        case 0x722: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->minstretcfgh;
            return true;
        }
        case 0x723: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent3h;
            return true;
        }
        case 0x724: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent4h;
            return true;
        }
        case 0x725: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent5h;
            return true;
        }
        case 0x726: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent6h;
            return true;
        }
        case 0x727: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent7h;
            return true;
        }
        case 0x728: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent8h;
            return true;
        }
        case 0x729: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent9h;
            return true;
        }
        case 0x72a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent10h;
            return true;
        }
        case 0x72b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent11h;
            return true;
        }
        case 0x72c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent12h;
            return true;
        }
        case 0x72d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent13h;
            return true;
        }
        case 0x72e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent14h;
            return true;
        }
        case 0x72f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent15h;
            return true;
        }
        case 0x730: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent16h;
            return true;
        }
        case 0x731: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent17h;
            return true;
        }
        case 0x732: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent18h;
            return true;
        }
        case 0x733: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent19h;
            return true;
        }
        case 0x734: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent20h;
            return true;
        }
        case 0x735: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent21h;
            return true;
        }
        case 0x736: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent22h;
            return true;
        }
        case 0x737: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent23h;
            return true;
        }
        case 0x738: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent24h;
            return true;
        }
        case 0x739: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent25h;
            return true;
        }
        case 0x73a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent26h;
            return true;
        }
        case 0x73b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent27h;
            return true;
        }
        case 0x73c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent28h;
            return true;
        }
        case 0x73d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent29h;
            return true;
        }
        case 0x73e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent30h;
            return true;
        }
        case 0x73f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmevent31h;
            return true;
        }
        case 0x340: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mscratch;
            return true;
        }
        case 0x341: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mepc;
            return true;
        }
        case 0x342: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mcause.raw;
            return true;
        }
        case 0x343: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mtval;
            return true;
        }
        case 0x344: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mip.raw;
            return true;
        }
        case 0x34a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mtinst;
            return true;
        }
        case 0x34b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mtval2;
            return true;
        }
        case 0x3a0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg0;
            return true;
        }
        case 0x3a1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg1;
            return true;
        }
        case 0x3a2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg2;
            return true;
        }
        case 0x3a3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg3;
            return true;
        }
        case 0x3a4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg4;
            return true;
        }
        case 0x3a5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg5;
            return true;
        }
        case 0x3a6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg6;
            return true;
        }
        case 0x3a7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg7;
            return true;
        }
        case 0x3a8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg8;
            return true;
        }
        case 0x3a9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg9;
            return true;
        }
        case 0x3aa: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg10;
            return true;
        }
        case 0x3ab: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg11;
            return true;
        }
        case 0x3ac: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg12;
            return true;
        }
        case 0x3ad: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg13;
            return true;
        }
        case 0x3ae: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg14;
            return true;
        }
        case 0x3af: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpcfg15;
            return true;
        }
        case 0x3b0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr0;
            return true;
        }
        case 0x3b1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr1;
            return true;
        }
        case 0x3b2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr2;
            return true;
        }
        case 0x3b3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr3;
            return true;
        }
        case 0x3b4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr4;
            return true;
        }
        case 0x3b5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr5;
            return true;
        }
        case 0x3b6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr6;
            return true;
        }
        case 0x3b7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr7;
            return true;
        }
        case 0x3b8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr8;
            return true;
        }
        case 0x3b9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr9;
            return true;
        }
        case 0x3ba: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr10;
            return true;
        }
        case 0x3bb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr11;
            return true;
        }
        case 0x3bc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr12;
            return true;
        }
        case 0x3bd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr13;
            return true;
        }
        case 0x3be: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr14;
            return true;
        }
        case 0x3bf: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr15;
            return true;
        }
        case 0x3c0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr16;
            return true;
        }
        case 0x3c1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr17;
            return true;
        }
        case 0x3c2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr18;
            return true;
        }
        case 0x3c3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr19;
            return true;
        }
        case 0x3c4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr20;
            return true;
        }
        case 0x3c5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr21;
            return true;
        }
        case 0x3c6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr22;
            return true;
        }
        case 0x3c7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr23;
            return true;
        }
        case 0x3c8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr24;
            return true;
        }
        case 0x3c9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr25;
            return true;
        }
        case 0x3ca: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr26;
            return true;
        }
        case 0x3cb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr27;
            return true;
        }
        case 0x3cc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr28;
            return true;
        }
        case 0x3cd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr29;
            return true;
        }
        case 0x3ce: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr30;
            return true;
        }
        case 0x3cf: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr31;
            return true;
        }
        case 0x3d0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr32;
            return true;
        }
        case 0x3d1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr33;
            return true;
        }
        case 0x3d2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr34;
            return true;
        }
        case 0x3d3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr35;
            return true;
        }
        case 0x3d4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr36;
            return true;
        }
        case 0x3d5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr37;
            return true;
        }
        case 0x3d6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr38;
            return true;
        }
        case 0x3d7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr39;
            return true;
        }
        case 0x3d8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr40;
            return true;
        }
        case 0x3d9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr41;
            return true;
        }
        case 0x3da: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr42;
            return true;
        }
        case 0x3db: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr43;
            return true;
        }
        case 0x3dc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr44;
            return true;
        }
        case 0x3dd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr45;
            return true;
        }
        case 0x3de: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr46;
            return true;
        }
        case 0x3df: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr47;
            return true;
        }
        case 0x3e0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr48;
            return true;
        }
        case 0x3e1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr49;
            return true;
        }
        case 0x3e2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr50;
            return true;
        }
        case 0x3e3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr51;
            return true;
        }
        case 0x3e4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr52;
            return true;
        }
        case 0x3e5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr53;
            return true;
        }
        case 0x3e6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr54;
            return true;
        }
        case 0x3e7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr55;
            return true;
        }
        case 0x3e8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr56;
            return true;
        }
        case 0x3e9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr57;
            return true;
        }
        case 0x3ea: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr58;
            return true;
        }
        case 0x3eb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr59;
            return true;
        }
        case 0x3ec: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr60;
            return true;
        }
        case 0x3ed: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr61;
            return true;
        }
        case 0x3ee: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr62;
            return true;
        }
        case 0x3ef: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->pmpaddr63;
            return true;
        }
        case 0x7a0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->tselect;
            return true;
        }
        case 0x7a4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->tinfo;
            return true;
        }
        case 0xb03: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter3;
            return true;
        }
        case 0xb04: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter4;
            return true;
        }
        case 0xb05: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter5;
            return true;
        }
        case 0xb06: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter6;
            return true;
        }
        case 0xb07: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter7;
            return true;
        }
        case 0xb08: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter8;
            return true;
        }
        case 0xb09: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter9;
            return true;
        }
        case 0xb0a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter10;
            return true;
        }
        case 0xb0b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter11;
            return true;
        }
        case 0xb0c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter12;
            return true;
        }
        case 0xb0d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter13;
            return true;
        }
        case 0xb0e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter14;
            return true;
        }
        case 0xb0f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter15;
            return true;
        }
        case 0xb10: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter16;
            return true;
        }
        case 0xb11: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter17;
            return true;
        }
        case 0xb12: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter18;
            return true;
        }
        case 0xb13: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter19;
            return true;
        }
        case 0xb14: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter20;
            return true;
        }
        case 0xb15: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter21;
            return true;
        }
        case 0xb16: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter22;
            return true;
        }
        case 0xb17: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter23;
            return true;
        }
        case 0xb18: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter24;
            return true;
        }
        case 0xb19: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter25;
            return true;
        }
        case 0xb1a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter26;
            return true;
        }
        case 0xb1b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter27;
            return true;
        }
        case 0xb1c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter28;
            return true;
        }
        case 0xb1d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter29;
            return true;
        }
        case 0xb1e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter30;
            return true;
        }
        case 0xb1f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter31;
            return true;
        }
        case 0xb83: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter3h;
            return true;
        }
        case 0xb84: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter4h;
            return true;
        }
        case 0xb85: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter5h;
            return true;
        }
        case 0xb86: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter6h;
            return true;
        }
        case 0xb87: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter7h;
            return true;
        }
        case 0xb88: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter8h;
            return true;
        }
        case 0xb89: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter9h;
            return true;
        }
        case 0xb8a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter10h;
            return true;
        }
        case 0xb8b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter11h;
            return true;
        }
        case 0xb8c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter12h;
            return true;
        }
        case 0xb8d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter13h;
            return true;
        }
        case 0xb8e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter14h;
            return true;
        }
        case 0xb8f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter15h;
            return true;
        }
        case 0xb90: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter16h;
            return true;
        }
        case 0xb91: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter17h;
            return true;
        }
        case 0xb92: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter18h;
            return true;
        }
        case 0xb93: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter19h;
            return true;
        }
        case 0xb94: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter20h;
            return true;
        }
        case 0xb95: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter21h;
            return true;
        }
        case 0xb96: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter22h;
            return true;
        }
        case 0xb97: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter23h;
            return true;
        }
        case 0xb98: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter24h;
            return true;
        }
        case 0xb99: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter25h;
            return true;
        }
        case 0xb9a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter26h;
            return true;
        }
        case 0xb9b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter27h;
            return true;
        }
        case 0xb9c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter28h;
            return true;
        }
        case 0xb9d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter29h;
            return true;
        }
        case 0xb9e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter30h;
            return true;
        }
        case 0xb9f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhpmcounter31h;
            return true;
        }
        case 0xc00: {
            CSR_CHECK_PRIV(RV32G_PRIV_USER);
            *data = csr->cycle;
            return true;
        }
        case 0xc01: {
            CSR_CHECK_PRIV(RV32G_PRIV_USER);
            *data = csr->time;
            return true;
        }
        case 0xc02: {
            CSR_CHECK_PRIV(RV32G_PRIV_USER);
            *data = csr->instret;
            return true;
        }
        case 0xc80: {
            CSR_CHECK_PRIV(RV32G_PRIV_USER);
            *data = csr->cycleh;
            return true;
        }
        case 0xc81: {
            CSR_CHECK_PRIV(RV32G_PRIV_USER);
            *data = csr->timeh;
            return true;
        }
        case 0xda0: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->scountovf;
            return true;
        }
        case 0xf11: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mvendorid;
            return true;
        }
        case 0xf12: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->marchid;
            return true;
        }
        case 0xf13: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mimpid;
            return true;
        }
        case 0xf14: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mhartid;
            return true;
        }
        case 0xfb0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mtopi;
            return true;
        }
        default:
            return false;
    }

    return false;
}

bool rv32g_csr_write(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data)
{
    switch (addr) {
        case 0x100: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sstatus.raw = data;
            return true;
        }
        case 0x104: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sie.raw = data;
            return true;
        }
        case 0x105: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->stvec.raw = data;
            return true;
        }
        case 0x106: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->scounteren.raw = data;
            return true;
        }
        case 0x10a: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->senvcfg.raw = data;
            return true;
        }
        case 0x140: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sscratch = data;
            return true;
        }
        case 0x141: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sepc = data;
            return true;
        }
        case 0x142: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->scause.raw = data;
            return true;
        }
        case 0x143: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->stval = data;
            return true;
        }
        case 0x144: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sip.raw = data;
            return true;
        }
        case 0x14d: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->stimecmp = data;
            return true;
        }
        case 0x15d: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->stimecmph = data;
            return true;
        }
        case 0x180: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->satp.raw = data;
            return true;
        }
        case 0x300: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mstatus.raw = data;
            return true;
        }
        case 0x301: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->misa.raw = data;
            return true;
        }
        case 0x302: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->medeleg.raw = data;
            return true;
        }
        case 0x303: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mideleg.raw = data;
            return true;
        }
        case 0x304: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mie.raw = data;
            return true;
        }
        case 0x305: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mtvec.raw = data;
            return true;
        }
        case 0x306: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mcounteren.raw = data;
            return true;
        }
        case 0x30a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->menvcfg.raw = data;
            return true;
        }
        case 0x310: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mstatush.raw = data;
            return true;
        }
        case 0x31a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->menvcfgh.raw = data;
            return true;
        }
        case 0x320: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mcountinhibit.raw = data;
            return true;
        }
        case 0x321: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mcyclecfg = data;
            return true;
        }
        case 0x322: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->minstretcfg = data;
            return true;
        }
        case 0x323: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent3 = data;
            return true;
        }
        case 0x324: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent4 = data;
            return true;
        }
        case 0x325: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent5 = data;
            return true;
        }
        case 0x326: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent6 = data;
            return true;
        }
        case 0x327: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent7 = data;
            return true;
        }
        case 0x328: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent8 = data;
            return true;
        }
        case 0x329: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent9 = data;
            return true;
        }
        case 0x32a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent10 = data;
            return true;
        }
        case 0x32b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent11 = data;
            return true;
        }
        case 0x32c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent12 = data;
            return true;
        }
        case 0x32d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent13 = data;
            return true;
        }
        case 0x32e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent14 = data;
            return true;
        }
        case 0x32f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent15 = data;
            return true;
        }
        case 0x330: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent16 = data;
            return true;
        }
        case 0x331: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent17 = data;
            return true;
        }
        case 0x332: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent18 = data;
            return true;
        }
        case 0x333: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent19 = data;
            return true;
        }
        case 0x334: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent20 = data;
            return true;
        }
        case 0x335: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent21 = data;
            return true;
        }
        case 0x336: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent22 = data;
            return true;
        }
        case 0x337: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent23 = data;
            return true;
        }
        case 0x338: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent24 = data;
            return true;
        }
        case 0x339: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent25 = data;
            return true;
        }
        case 0x33a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent26 = data;
            return true;
        }
        case 0x33b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent27 = data;
            return true;
        }
        case 0x33c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent28 = data;
            return true;
        }
        case 0x33d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent29 = data;
            return true;
        }
        case 0x33e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent30 = data;
            return true;
        }
        case 0x33f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent31 = data;
            return true;
        }
        case 0x721: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mcyclecfgh = data;
            return true;
        }
        case 0x722: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->minstretcfgh = data;
            return true;
        }
        case 0x723: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent3h = data;
            return true;
        }
        case 0x724: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent4h = data;
            return true;
        }
        case 0x725: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent5h = data;
            return true;
        }
        case 0x726: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent6h = data;
            return true;
        }
        case 0x727: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent7h = data;
            return true;
        }
        case 0x728: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent8h = data;
            return true;
        }
        case 0x729: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent9h = data;
            return true;
        }
        case 0x72a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent10h = data;
            return true;
        }
        case 0x72b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent11h = data;
            return true;
        }
        case 0x72c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent12h = data;
            return true;
        }
        case 0x72d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent13h = data;
            return true;
        }
        case 0x72e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent14h = data;
            return true;
        }
        case 0x72f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent15h = data;
            return true;
        }
        case 0x730: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent16h = data;
            return true;
        }
        case 0x731: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent17h = data;
            return true;
        }
        case 0x732: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent18h = data;
            return true;
        }
        case 0x733: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent19h = data;
            return true;
        }
        case 0x734: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent20h = data;
            return true;
        }
        case 0x735: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent21h = data;
            return true;
        }
        case 0x736: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent22h = data;
            return true;
        }
        case 0x737: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent23h = data;
            return true;
        }
        case 0x738: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent24h = data;
            return true;
        }
        case 0x739: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent25h = data;
            return true;
        }
        case 0x73a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent26h = data;
            return true;
        }
        case 0x73b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent27h = data;
            return true;
        }
        case 0x73c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent28h = data;
            return true;
        }
        case 0x73d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent29h = data;
            return true;
        }
        case 0x73e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent30h = data;
            return true;
        }
        case 0x73f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmevent31h = data;
            return true;
        }
        case 0x340: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mscratch = data;
            return true;
        }
        case 0x341: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mepc = data;
            return true;
        }
        case 0x342: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mcause.raw = data;
            return true;
        }
        case 0x343: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mtval = data;
            return true;
        }
        case 0x344: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mip.raw = data;
            return true;
        }
        case 0x34a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mtinst = data;
            return true;
        }
        case 0x34b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mtval2 = data;
            return true;
        }
        case 0x3a0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg0 = data;
            return true;
        }
        case 0x3a1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg1 = data;
            return true;
        }
        case 0x3a2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg2 = data;
            return true;
        }
        case 0x3a3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg3 = data;
            return true;
        }
        case 0x3a4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg4 = data;
            return true;
        }
        case 0x3a5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg5 = data;
            return true;
        }
        case 0x3a6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg6 = data;
            return true;
        }
        case 0x3a7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg7 = data;
            return true;
        }
        case 0x3a8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg8 = data;
            return true;
        }
        case 0x3a9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg9 = data;
            return true;
        }
        case 0x3aa: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg10 = data;
            return true;
        }
        case 0x3ab: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg11 = data;
            return true;
        }
        case 0x3ac: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg12 = data;
            return true;
        }
        case 0x3ad: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg13 = data;
            return true;
        }
        case 0x3ae: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg14 = data;
            return true;
        }
        case 0x3af: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpcfg15 = data;
            return true;
        }
        case 0x3b0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr0 = data;
            return true;
        }
        case 0x3b1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr1 = data;
            return true;
        }
        case 0x3b2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr2 = data;
            return true;
        }
        case 0x3b3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr3 = data;
            return true;
        }
        case 0x3b4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr4 = data;
            return true;
        }
        case 0x3b5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr5 = data;
            return true;
        }
        case 0x3b6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr6 = data;
            return true;
        }
        case 0x3b7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr7 = data;
            return true;
        }
        case 0x3b8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr8 = data;
            return true;
        }
        case 0x3b9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr9 = data;
            return true;
        }
        case 0x3ba: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr10 = data;
            return true;
        }
        case 0x3bb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr11 = data;
            return true;
        }
        case 0x3bc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr12 = data;
            return true;
        }
        case 0x3bd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr13 = data;
            return true;
        }
        case 0x3be: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr14 = data;
            return true;
        }
        case 0x3bf: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr15 = data;
            return true;
        }
        case 0x3c0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr16 = data;
            return true;
        }
        case 0x3c1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr17 = data;
            return true;
        }
        case 0x3c2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr18 = data;
            return true;
        }
        case 0x3c3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr19 = data;
            return true;
        }
        case 0x3c4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr20 = data;
            return true;
        }
        case 0x3c5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr21 = data;
            return true;
        }
        case 0x3c6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr22 = data;
            return true;
        }
        case 0x3c7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr23 = data;
            return true;
        }
        case 0x3c8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr24 = data;
            return true;
        }
        case 0x3c9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr25 = data;
            return true;
        }
        case 0x3ca: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr26 = data;
            return true;
        }
        case 0x3cb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr27 = data;
            return true;
        }
        case 0x3cc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr28 = data;
            return true;
        }
        case 0x3cd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr29 = data;
            return true;
        }
        case 0x3ce: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr30 = data;
            return true;
        }
        case 0x3cf: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr31 = data;
            return true;
        }
        case 0x3d0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr32 = data;
            return true;
        }
        case 0x3d1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr33 = data;
            return true;
        }
        case 0x3d2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr34 = data;
            return true;
        }
        case 0x3d3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr35 = data;
            return true;
        }
        case 0x3d4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr36 = data;
            return true;
        }
        case 0x3d5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr37 = data;
            return true;
        }
        case 0x3d6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr38 = data;
            return true;
        }
        case 0x3d7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr39 = data;
            return true;
        }
        case 0x3d8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr40 = data;
            return true;
        }
        case 0x3d9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr41 = data;
            return true;
        }
        case 0x3da: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr42 = data;
            return true;
        }
        case 0x3db: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr43 = data;
            return true;
        }
        case 0x3dc: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr44 = data;
            return true;
        }
        case 0x3dd: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr45 = data;
            return true;
        }
        case 0x3de: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr46 = data;
            return true;
        }
        case 0x3df: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr47 = data;
            return true;
        }
        case 0x3e0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr48 = data;
            return true;
        }
        case 0x3e1: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr49 = data;
            return true;
        }
        case 0x3e2: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr50 = data;
            return true;
        }
        case 0x3e3: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr51 = data;
            return true;
        }
        case 0x3e4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr52 = data;
            return true;
        }
        case 0x3e5: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr53 = data;
            return true;
        }
        case 0x3e6: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr54 = data;
            return true;
        }
        case 0x3e7: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr55 = data;
            return true;
        }
        case 0x3e8: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr56 = data;
            return true;
        }
        case 0x3e9: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr57 = data;
            return true;
        }
        case 0x3ea: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr58 = data;
            return true;
        }
        case 0x3eb: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr59 = data;
            return true;
        }
        case 0x3ec: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr60 = data;
            return true;
        }
        case 0x3ed: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr61 = data;
            return true;
        }
        case 0x3ee: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr62 = data;
            return true;
        }
        case 0x3ef: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->pmpaddr63 = data;
            return true;
        }
        case 0x7a0: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->tselect = data;
            return true;
        }
        case 0x7a4: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->tinfo = data;
            return true;
        }
        case 0xb03: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter3 = data;
            return true;
        }
        case 0xb04: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter4 = data;
            return true;
        }
        case 0xb05: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter5 = data;
            return true;
        }
        case 0xb06: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter6 = data;
            return true;
        }
        case 0xb07: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter7 = data;
            return true;
        }
        case 0xb08: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter8 = data;
            return true;
        }
        case 0xb09: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter9 = data;
            return true;
        }
        case 0xb0a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter10 = data;
            return true;
        }
        case 0xb0b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter11 = data;
            return true;
        }
        case 0xb0c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter12 = data;
            return true;
        }
        case 0xb0d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter13 = data;
            return true;
        }
        case 0xb0e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter14 = data;
            return true;
        }
        case 0xb0f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter15 = data;
            return true;
        }
        case 0xb10: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter16 = data;
            return true;
        }
        case 0xb11: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter17 = data;
            return true;
        }
        case 0xb12: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter18 = data;
            return true;
        }
        case 0xb13: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter19 = data;
            return true;
        }
        case 0xb14: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter20 = data;
            return true;
        }
        case 0xb15: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter21 = data;
            return true;
        }
        case 0xb16: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter22 = data;
            return true;
        }
        case 0xb17: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter23 = data;
            return true;
        }
        case 0xb18: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter24 = data;
            return true;
        }
        case 0xb19: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter25 = data;
            return true;
        }
        case 0xb1a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter26 = data;
            return true;
        }
        case 0xb1b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter27 = data;
            return true;
        }
        case 0xb1c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter28 = data;
            return true;
        }
        case 0xb1d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter29 = data;
            return true;
        }
        case 0xb1e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter30 = data;
            return true;
        }
        case 0xb1f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter31 = data;
            return true;
        }
        case 0xb83: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter3h = data;
            return true;
        }
        case 0xb84: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter4h = data;
            return true;
        }
        case 0xb85: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter5h = data;
            return true;
        }
        case 0xb86: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter6h = data;
            return true;
        }
        case 0xb87: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter7h = data;
            return true;
        }
        case 0xb88: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter8h = data;
            return true;
        }
        case 0xb89: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter9h = data;
            return true;
        }
        case 0xb8a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter10h = data;
            return true;
        }
        case 0xb8b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter11h = data;
            return true;
        }
        case 0xb8c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter12h = data;
            return true;
        }
        case 0xb8d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter13h = data;
            return true;
        }
        case 0xb8e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter14h = data;
            return true;
        }
        case 0xb8f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter15h = data;
            return true;
        }
        case 0xb90: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter16h = data;
            return true;
        }
        case 0xb91: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter17h = data;
            return true;
        }
        case 0xb92: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter18h = data;
            return true;
        }
        case 0xb93: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter19h = data;
            return true;
        }
        case 0xb94: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter20h = data;
            return true;
        }
        case 0xb95: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter21h = data;
            return true;
        }
        case 0xb96: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter22h = data;
            return true;
        }
        case 0xb97: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter23h = data;
            return true;
        }
        case 0xb98: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter24h = data;
            return true;
        }
        case 0xb99: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter25h = data;
            return true;
        }
        case 0xb9a: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter26h = data;
            return true;
        }
        case 0xb9b: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter27h = data;
            return true;
        }
        case 0xb9c: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter28h = data;
            return true;
        }
        case 0xb9d: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter29h = data;
            return true;
        }
        case 0xb9e: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter30h = data;
            return true;
        }
        case 0xb9f: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mhpmcounter31h = data;
            return true;
        }
        default:
            return false;
    }

    return false;
}

const char *rv32g_csr_name(u32 addr)
{
    switch (addr) {
        case 0x100:
            return "sstatus";
        case 0x104:
            return "sie";
        case 0x105:
            return "stvec";
        case 0x106:
            return "scounteren";
        case 0x10a:
            return "senvcfg";
        case 0x140:
            return "sscratch";
        case 0x141:
            return "sepc";
        case 0x142:
            return "scause";
        case 0x143:
            return "stval";
        case 0x144:
            return "sip";
        case 0x14d:
            return "stimecmp";
        case 0x15d:
            return "stimecmph";
        case 0x180:
            return "satp";
        case 0x300:
            return "mstatus";
        case 0x301:
            return "misa";
        case 0x302:
            return "medeleg";
        case 0x303:
            return "mideleg";
        case 0x304:
            return "mie";
        case 0x305:
            return "mtvec";
        case 0x306:
            return "mcounteren";
        case 0x30a:
            return "menvcfg";
        case 0x310:
            return "mstatush";
        case 0x31a:
            return "menvcfgh";
        case 0x320:
            return "mcountinhibit";
        case 0x321:
            return "mcyclecfg";
        case 0x322:
            return "minstretcfg";
        case 0x323:
            return "mhpmevent3";
        case 0x324:
            return "mhpmevent4";
        case 0x325:
            return "mhpmevent5";
        case 0x326:
            return "mhpmevent6";
        case 0x327:
            return "mhpmevent7";
        case 0x328:
            return "mhpmevent8";
        case 0x329:
            return "mhpmevent9";
        case 0x32a:
            return "mhpmevent10";
        case 0x32b:
            return "mhpmevent11";
        case 0x32c:
            return "mhpmevent12";
        case 0x32d:
            return "mhpmevent13";
        case 0x32e:
            return "mhpmevent14";
        case 0x32f:
            return "mhpmevent15";
        case 0x330:
            return "mhpmevent16";
        case 0x331:
            return "mhpmevent17";
        case 0x332:
            return "mhpmevent18";
        case 0x333:
            return "mhpmevent19";
        case 0x334:
            return "mhpmevent20";
        case 0x335:
            return "mhpmevent21";
        case 0x336:
            return "mhpmevent22";
        case 0x337:
            return "mhpmevent23";
        case 0x338:
            return "mhpmevent24";
        case 0x339:
            return "mhpmevent25";
        case 0x33a:
            return "mhpmevent26";
        case 0x33b:
            return "mhpmevent27";
        case 0x33c:
            return "mhpmevent28";
        case 0x33d:
            return "mhpmevent29";
        case 0x33e:
            return "mhpmevent30";
        case 0x33f:
            return "mhpmevent31";
        case 0x721:
            return "mcyclecfgh";
        case 0x722:
            return "minstretcfgh";
        case 0x723:
            return "mhpmevent3h";
        case 0x724:
            return "mhpmevent4h";
        case 0x725:
            return "mhpmevent5h";
        case 0x726:
            return "mhpmevent6h";
        case 0x727:
            return "mhpmevent7h";
        case 0x728:
            return "mhpmevent8h";
        case 0x729:
            return "mhpmevent9h";
        case 0x72a:
            return "mhpmevent10h";
        case 0x72b:
            return "mhpmevent11h";
        case 0x72c:
            return "mhpmevent12h";
        case 0x72d:
            return "mhpmevent13h";
        case 0x72e:
            return "mhpmevent14h";
        case 0x72f:
            return "mhpmevent15h";
        case 0x730:
            return "mhpmevent16h";
        case 0x731:
            return "mhpmevent17h";
        case 0x732:
            return "mhpmevent18h";
        case 0x733:
            return "mhpmevent19h";
        case 0x734:
            return "mhpmevent20h";
        case 0x735:
            return "mhpmevent21h";
        case 0x736:
            return "mhpmevent22h";
        case 0x737:
            return "mhpmevent23h";
        case 0x738:
            return "mhpmevent24h";
        case 0x739:
            return "mhpmevent25h";
        case 0x73a:
            return "mhpmevent26h";
        case 0x73b:
            return "mhpmevent27h";
        case 0x73c:
            return "mhpmevent28h";
        case 0x73d:
            return "mhpmevent29h";
        case 0x73e:
            return "mhpmevent30h";
        case 0x73f:
            return "mhpmevent31h";
        case 0x340:
            return "mscratch";
        case 0x341:
            return "mepc";
        case 0x342:
            return "mcause";
        case 0x343:
            return "mtval";
        case 0x344:
            return "mip";
        case 0x34a:
            return "mtinst";
        case 0x34b:
            return "mtval2";
        case 0x3a0:
            return "pmpcfg0";
        case 0x3a1:
            return "pmpcfg1";
        case 0x3a2:
            return "pmpcfg2";
        case 0x3a3:
            return "pmpcfg3";
        case 0x3a4:
            return "pmpcfg4";
        case 0x3a5:
            return "pmpcfg5";
        case 0x3a6:
            return "pmpcfg6";
        case 0x3a7:
            return "pmpcfg7";
        case 0x3a8:
            return "pmpcfg8";
        case 0x3a9:
            return "pmpcfg9";
        case 0x3aa:
            return "pmpcfg10";
        case 0x3ab:
            return "pmpcfg11";
        case 0x3ac:
            return "pmpcfg12";
        case 0x3ad:
            return "pmpcfg13";
        case 0x3ae:
            return "pmpcfg14";
        case 0x3af:
            return "pmpcfg15";
        case 0x3b0:
            return "pmpaddr0";
        case 0x3b1:
            return "pmpaddr1";
        case 0x3b2:
            return "pmpaddr2";
        case 0x3b3:
            return "pmpaddr3";
        case 0x3b4:
            return "pmpaddr4";
        case 0x3b5:
            return "pmpaddr5";
        case 0x3b6:
            return "pmpaddr6";
        case 0x3b7:
            return "pmpaddr7";
        case 0x3b8:
            return "pmpaddr8";
        case 0x3b9:
            return "pmpaddr9";
        case 0x3ba:
            return "pmpaddr10";
        case 0x3bb:
            return "pmpaddr11";
        case 0x3bc:
            return "pmpaddr12";
        case 0x3bd:
            return "pmpaddr13";
        case 0x3be:
            return "pmpaddr14";
        case 0x3bf:
            return "pmpaddr15";
        case 0x3c0:
            return "pmpaddr16";
        case 0x3c1:
            return "pmpaddr17";
        case 0x3c2:
            return "pmpaddr18";
        case 0x3c3:
            return "pmpaddr19";
        case 0x3c4:
            return "pmpaddr20";
        case 0x3c5:
            return "pmpaddr21";
        case 0x3c6:
            return "pmpaddr22";
        case 0x3c7:
            return "pmpaddr23";
        case 0x3c8:
            return "pmpaddr24";
        case 0x3c9:
            return "pmpaddr25";
        case 0x3ca:
            return "pmpaddr26";
        case 0x3cb:
            return "pmpaddr27";
        case 0x3cc:
            return "pmpaddr28";
        case 0x3cd:
            return "pmpaddr29";
        case 0x3ce:
            return "pmpaddr30";
        case 0x3cf:
            return "pmpaddr31";
        case 0x3d0:
            return "pmpaddr32";
        case 0x3d1:
            return "pmpaddr33";
        case 0x3d2:
            return "pmpaddr34";
        case 0x3d3:
            return "pmpaddr35";
        case 0x3d4:
            return "pmpaddr36";
        case 0x3d5:
            return "pmpaddr37";
        case 0x3d6:
            return "pmpaddr38";
        case 0x3d7:
            return "pmpaddr39";
        case 0x3d8:
            return "pmpaddr40";
        case 0x3d9:
            return "pmpaddr41";
        case 0x3da:
            return "pmpaddr42";
        case 0x3db:
            return "pmpaddr43";
        case 0x3dc:
            return "pmpaddr44";
        case 0x3dd:
            return "pmpaddr45";
        case 0x3de:
            return "pmpaddr46";
        case 0x3df:
            return "pmpaddr47";
        case 0x3e0:
            return "pmpaddr48";
        case 0x3e1:
            return "pmpaddr49";
        case 0x3e2:
            return "pmpaddr50";
        case 0x3e3:
            return "pmpaddr51";
        case 0x3e4:
            return "pmpaddr52";
        case 0x3e5:
            return "pmpaddr53";
        case 0x3e6:
            return "pmpaddr54";
        case 0x3e7:
            return "pmpaddr55";
        case 0x3e8:
            return "pmpaddr56";
        case 0x3e9:
            return "pmpaddr57";
        case 0x3ea:
            return "pmpaddr58";
        case 0x3eb:
            return "pmpaddr59";
        case 0x3ec:
            return "pmpaddr60";
        case 0x3ed:
            return "pmpaddr61";
        case 0x3ee:
            return "pmpaddr62";
        case 0x3ef:
            return "pmpaddr63";
        case 0x7a0:
            return "tselect";
        case 0x7a4:
            return "tinfo";
        case 0xb03:
            return "mhpmcounter3";
        case 0xb04:
            return "mhpmcounter4";
        case 0xb05:
            return "mhpmcounter5";
        case 0xb06:
            return "mhpmcounter6";
        case 0xb07:
            return "mhpmcounter7";
        case 0xb08:
            return "mhpmcounter8";
        case 0xb09:
            return "mhpmcounter9";
        case 0xb0a:
            return "mhpmcounter10";
        case 0xb0b:
            return "mhpmcounter11";
        case 0xb0c:
            return "mhpmcounter12";
        case 0xb0d:
            return "mhpmcounter13";
        case 0xb0e:
            return "mhpmcounter14";
        case 0xb0f:
            return "mhpmcounter15";
        case 0xb10:
            return "mhpmcounter16";
        case 0xb11:
            return "mhpmcounter17";
        case 0xb12:
            return "mhpmcounter18";
        case 0xb13:
            return "mhpmcounter19";
        case 0xb14:
            return "mhpmcounter20";
        case 0xb15:
            return "mhpmcounter21";
        case 0xb16:
            return "mhpmcounter22";
        case 0xb17:
            return "mhpmcounter23";
        case 0xb18:
            return "mhpmcounter24";
        case 0xb19:
            return "mhpmcounter25";
        case 0xb1a:
            return "mhpmcounter26";
        case 0xb1b:
            return "mhpmcounter27";
        case 0xb1c:
            return "mhpmcounter28";
        case 0xb1d:
            return "mhpmcounter29";
        case 0xb1e:
            return "mhpmcounter30";
        case 0xb1f:
            return "mhpmcounter31";
        case 0xb83:
            return "mhpmcounter3h";
        case 0xb84:
            return "mhpmcounter4h";
        case 0xb85:
            return "mhpmcounter5h";
        case 0xb86:
            return "mhpmcounter6h";
        case 0xb87:
            return "mhpmcounter7h";
        case 0xb88:
            return "mhpmcounter8h";
        case 0xb89:
            return "mhpmcounter9h";
        case 0xb8a:
            return "mhpmcounter10h";
        case 0xb8b:
            return "mhpmcounter11h";
        case 0xb8c:
            return "mhpmcounter12h";
        case 0xb8d:
            return "mhpmcounter13h";
        case 0xb8e:
            return "mhpmcounter14h";
        case 0xb8f:
            return "mhpmcounter15h";
        case 0xb90:
            return "mhpmcounter16h";
        case 0xb91:
            return "mhpmcounter17h";
        case 0xb92:
            return "mhpmcounter18h";
        case 0xb93:
            return "mhpmcounter19h";
        case 0xb94:
            return "mhpmcounter20h";
        case 0xb95:
            return "mhpmcounter21h";
        case 0xb96:
            return "mhpmcounter22h";
        case 0xb97:
            return "mhpmcounter23h";
        case 0xb98:
            return "mhpmcounter24h";
        case 0xb99:
            return "mhpmcounter25h";
        case 0xb9a:
            return "mhpmcounter26h";
        case 0xb9b:
            return "mhpmcounter27h";
        case 0xb9c:
            return "mhpmcounter28h";
        case 0xb9d:
            return "mhpmcounter29h";
        case 0xb9e:
            return "mhpmcounter30h";
        case 0xb9f:
            return "mhpmcounter31h";
        case 0xc00:
            return "cycle";
        case 0xc01:
            return "time";
        case 0xc02:
            return "instret";
        case 0xc80:
            return "cycleh";
        case 0xc81:
            return "timeh";
        case 0xda0:
            return "scountovf";
        case 0xf11:
            return "mvendorid";
        case 0xf12:
            return "marchid";
        case 0xf13:
            return "mimpid";
        case 0xf14:
            return "mhartid";
        case 0xfb0:
            return "mtopi";
        default:
            return "unknown";
    }

    return "unknown";
}
