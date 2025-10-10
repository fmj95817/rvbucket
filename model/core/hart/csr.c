#include "csr.h"

#define CSR_CHECK_PRIV(p) if (priv < p) { return false; }

void csr_reset(csr_t *csr)
{
    csr->sstatus.raw = 0;
    csr->sie = 0;
    csr->stvec.raw = 0;
    csr->scounteren.raw = 0;
    csr->senvcfg.raw = 0;
    csr->sscratch = 0;
    csr->sepc = 0;
    csr->scause.raw = 0;
    csr->stval = 0;
    csr->sip = 0;
    csr->stimecmp = 0;
    csr->stimecmph = 0;
    csr->satp.raw = 0;
    csr->mstatus.raw = 0;
    csr->misa.raw = RV32G_CSR_MISA_I_BIT | RV32G_CSR_MISA_M_BIT | RV32G_CSR_MISA_A_BIT | RV32G_CSR_MISA_U_BIT | RV32G_CSR_MISA_S_BIT;
    csr->medeleg = 0;
    csr->mideleg = 0;
    csr->mie = 0;
    csr->mtvec.raw = 0;
    csr->mcounteren.raw = 0;
    csr->menvcfg.raw = 0;
    csr->mstatush.raw = 0;
    csr->menvcfgh.raw = 0;
    csr->mcountinhibit.raw = 0;
    csr->mcyclecfg = 0;
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
    csr->mscratch = 0;
    csr->mepc = 0;
    csr->mcause.raw = 0;
    csr->mtval = 0;
    csr->mip = 0;
    csr->mtinst = 0;
    csr->mtval2 = 0;
    csr->pmpcfg0 = 0;
    csr->pmpcfg1 = 0;
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

bool csr_read(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data)
{
    switch (addr) {
        case 0x100: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sstatus.raw;
            return true;
        }
        case 0x104: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            *data = csr->sie;
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
            *data = csr->sip;
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
            *data = csr->medeleg;
            return true;
        }
        case 0x303: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mideleg;
            return true;
        }
        case 0x304: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            *data = csr->mie;
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
            *data = csr->mip;
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

bool csr_write(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data)
{
    switch (addr) {
        case 0x100: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sstatus.raw = data;
            return true;
        }
        case 0x104: {
            CSR_CHECK_PRIV(RV32G_PRIV_SUPERVISOR);
            csr->sie = data;
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
            csr->sip = data;
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
            csr->medeleg = data;
            return true;
        }
        case 0x303: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mideleg = data;
            return true;
        }
        case 0x304: {
            CSR_CHECK_PRIV(RV32G_PRIV_MACHINE);
            csr->mie = data;
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
            csr->mip = data;
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
        default:
            return false;
    }

    return false;
}

const char *csr_name(u32 addr)
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
