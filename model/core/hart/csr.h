#ifndef CSR_H
#define CSR_H

#include "base/types.h"
#include "isa.h"

typedef enum rv32g_csr_mtvec_mode {
    RV32G_CSR_MTVEC_MODE_DIRECT = 0u,
    RV32G_CSR_MTVEC_MODE_VECTORED = 1u,
} rv32g_csr_mtvec_mode_t;

typedef enum rv32g_csr_mcause_expt {
    RV32G_CSR_MCAUSE_EXPT_INST_ADDR_MISALIGNED = 0u,
    RV32G_CSR_MCAUSE_EXPT_INST_ACCESS_FAULT = 1u,
    RV32G_CSR_MCAUSE_EXPT_ILLEGAL_INST = 2u,
    RV32G_CSR_MCAUSE_EXPT_BREAKPOINT = 3u,
    RV32G_CSR_MCAUSE_EXPT_LOAD_ADDR_MISALIGNED = 4u,
    RV32G_CSR_MCAUSE_EXPT_LOAD_ACCESS_FAULT = 5u,
    RV32G_CSR_MCAUSE_EXPT_STORE_AMO_ADDR_MISALIGNED = 6u,
    RV32G_CSR_MCAUSE_EXPT_STORE_AMO_ACCESS_FAULT = 7u,
    RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_U = 8u,
    RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_S = 9u,
    RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_M = 11u,
    RV32G_CSR_MCAUSE_EXPT_INST_PAGE_FAULT = 12u,
    RV32G_CSR_MCAUSE_EXPT_LOAD_PAGE_FAULT = 13u,
    RV32G_CSR_MCAUSE_EXPT_STORE_AMO_PAGE_FAULT = 15u,
    RV32G_CSR_MCAUSE_EXPT_DOUBLE_TRAP = 16u,
    RV32G_CSR_MCAUSE_EXPT_SW_CHECK = 18u,
    RV32G_CSR_MCAUSE_EXPT_HW_ERROR = 19u,
} rv32g_csr_mcause_expt_t;

typedef enum rv32g_csr_mcause_intr {
    RV32G_CSR_MCAUSE_INTR_S_MODE_SW = 1u,
    RV32G_CSR_MCAUSE_INTR_M_MODE_SW = 3u,
    RV32G_CSR_MCAUSE_INTR_S_MODE_TIMER = 5u,
    RV32G_CSR_MCAUSE_INTR_M_MODE_TIMER = 7u,
    RV32G_CSR_MCAUSE_INTR_S_MODE_EXT = 9u,
    RV32G_CSR_MCAUSE_INTR_M_MODE_EXT = 11u,
    RV32G_CSR_MCAUSE_INTR_COUNTER_OVERFLOW = 13u,
} rv32g_csr_mcause_intr_t;

typedef enum rv32g_csr_stvec_mode {
    RV32G_CSR_STVEC_MODE_DIRECT = 0u,
    RV32G_CSR_STVEC_MODE_VECTORED = 1u,
} rv32g_csr_stvec_mode_t;

typedef enum rv32g_csr_scause_expt {
    RV32G_CSR_SCAUSE_EXPT_INST_ADDR_MISALIGNED = 0u,
    RV32G_CSR_SCAUSE_EXPT_INST_ACCESS_FAULT = 1u,
    RV32G_CSR_SCAUSE_EXPT_ILLEGAL_INST = 2u,
    RV32G_CSR_SCAUSE_EXPT_BREAKPOINT = 3u,
    RV32G_CSR_SCAUSE_EXPT_LOAD_ADDR_MISALIGNED = 4u,
    RV32G_CSR_SCAUSE_EXPT_LOAD_ACCESS_FAULT = 5u,
    RV32G_CSR_SCAUSE_EXPT_STORE_AMO_ADDR_MISALIGNED = 6u,
    RV32G_CSR_SCAUSE_EXPT_STORE_AMO_ACCESS_FAULT = 7u,
    RV32G_CSR_SCAUSE_EXPT_ECALL_FROM_U = 8u,
    RV32G_CSR_SCAUSE_EXPT_ECALL_FROM_S = 9u,
    RV32G_CSR_SCAUSE_EXPT_INST_PAGE_FAULT = 12u,
    RV32G_CSR_SCAUSE_EXPT_LOAD_PAGE_FAULT = 13u,
    RV32G_CSR_SCAUSE_EXPT_STORE_AMO_PAGE_FAULT = 15u,
    RV32G_CSR_SCAUSE_EXPT_SW_CHECK = 18u,
    RV32G_CSR_SCAUSE_EXPT_HW_ERROR = 19u,
} rv32g_csr_scause_expt_t;

typedef enum rv32g_csr_scause_intr {
    RV32G_CSR_SCAUSE_INTR_S_MODE_SW = 1u,
    RV32G_CSR_SCAUSE_INTR_S_MODE_TIMER = 5u,
    RV32G_CSR_SCAUSE_INTR_S_MODE_EXT = 9u,
    RV32G_CSR_SCAUSE_INTR_COUNTER_OVERFLOW = 13u,
} rv32g_csr_scause_intr_t;

#define RV32G_CSR_SSTATUS_SIE_BIT (1u << 1)
#define RV32G_CSR_SSTATUS_SPIE_BIT (1u << 5)
#define RV32G_CSR_SSTATUS_UBE_BIT (1u << 6)
#define RV32G_CSR_SSTATUS_SPP_BIT (1u << 8)
#define RV32G_CSR_SSTATUS_SUM_BIT (1u << 18)
#define RV32G_CSR_SSTATUS_MXR_BIT (1u << 19)
#define RV32G_CSR_SSTATUS_SPELP_BIT (1u << 23)
#define RV32G_CSR_SSTATUS_SDT_BIT (1u << 24)
#define RV32G_CSR_SSTATUS_SD_BIT (1u << 31)

#define RV32G_CSR_SCOUNTEREN_CY_BIT (1u << 0)
#define RV32G_CSR_SCOUNTEREN_TM_BIT (1u << 1)
#define RV32G_CSR_SCOUNTEREN_IR_BIT (1u << 2)
#define RV32G_CSR_SCOUNTEREN_HPM3_BIT (1u << 3)
#define RV32G_CSR_SCOUNTEREN_HPM4_BIT (1u << 4)
#define RV32G_CSR_SCOUNTEREN_HPM5_BIT (1u << 5)
#define RV32G_CSR_SCOUNTEREN_HPM6_BIT (1u << 6)
#define RV32G_CSR_SCOUNTEREN_HPM7_BIT (1u << 7)
#define RV32G_CSR_SCOUNTEREN_HPM8_BIT (1u << 8)
#define RV32G_CSR_SCOUNTEREN_HPM9_BIT (1u << 9)
#define RV32G_CSR_SCOUNTEREN_HPM10_BIT (1u << 10)
#define RV32G_CSR_SCOUNTEREN_HPM11_BIT (1u << 11)
#define RV32G_CSR_SCOUNTEREN_HPM12_BIT (1u << 12)
#define RV32G_CSR_SCOUNTEREN_HPM13_BIT (1u << 13)
#define RV32G_CSR_SCOUNTEREN_HPM14_BIT (1u << 14)
#define RV32G_CSR_SCOUNTEREN_HPM15_BIT (1u << 15)
#define RV32G_CSR_SCOUNTEREN_HPM16_BIT (1u << 16)
#define RV32G_CSR_SCOUNTEREN_HPM17_BIT (1u << 17)
#define RV32G_CSR_SCOUNTEREN_HPM18_BIT (1u << 18)
#define RV32G_CSR_SCOUNTEREN_HPM19_BIT (1u << 19)
#define RV32G_CSR_SCOUNTEREN_HPM20_BIT (1u << 20)
#define RV32G_CSR_SCOUNTEREN_HPM21_BIT (1u << 21)
#define RV32G_CSR_SCOUNTEREN_HPM22_BIT (1u << 22)
#define RV32G_CSR_SCOUNTEREN_HPM23_BIT (1u << 23)
#define RV32G_CSR_SCOUNTEREN_HPM24_BIT (1u << 24)
#define RV32G_CSR_SCOUNTEREN_HPM25_BIT (1u << 25)
#define RV32G_CSR_SCOUNTEREN_HPM26_BIT (1u << 26)
#define RV32G_CSR_SCOUNTEREN_HPM27_BIT (1u << 27)
#define RV32G_CSR_SCOUNTEREN_HPM28_BIT (1u << 28)
#define RV32G_CSR_SCOUNTEREN_HPM29_BIT (1u << 29)
#define RV32G_CSR_SCOUNTEREN_HPM30_BIT (1u << 30)
#define RV32G_CSR_SCOUNTEREN_HPM31_BIT (1u << 31)

#define RV32G_CSR_SENVCFG_FIOM_BIT (1u << 0)
#define RV32G_CSR_SENVCFG_LPE_BIT (1u << 2)
#define RV32G_CSR_SENVCFG_SSE_BIT (1u << 3)
#define RV32G_CSR_SENVCFG_CBCFE_BIT (1u << 6)
#define RV32G_CSR_SENVCFG_CBZE_BIT (1u << 7)

#define RV32G_CSR_SCAUSE_INTERRUPT_BIT (1u << 31)

#define RV32G_CSR_SATP_MODE_BIT (1u << 31)

#define RV32G_CSR_MSTATUS_SIE_BIT (1u << 1)
#define RV32G_CSR_MSTATUS_MIE_BIT (1u << 3)
#define RV32G_CSR_MSTATUS_SPIE_BIT (1u << 5)
#define RV32G_CSR_MSTATUS_UBE_BIT (1u << 6)
#define RV32G_CSR_MSTATUS_MPIE_BIT (1u << 7)
#define RV32G_CSR_MSTATUS_SPP_BIT (1u << 8)
#define RV32G_CSR_MSTATUS_MPRV_BIT (1u << 17)
#define RV32G_CSR_MSTATUS_SUM_BIT (1u << 18)
#define RV32G_CSR_MSTATUS_MXR_BIT (1u << 19)
#define RV32G_CSR_MSTATUS_TVM_BIT (1u << 20)
#define RV32G_CSR_MSTATUS_TW_BIT (1u << 21)
#define RV32G_CSR_MSTATUS_TSR_BIT (1u << 22)
#define RV32G_CSR_MSTATUS_SPELP_BIT (1u << 23)
#define RV32G_CSR_MSTATUS_STD_BIT (1u << 24)
#define RV32G_CSR_MSTATUS_SD_BIT (1u << 31)

#define RV32G_CSR_MISA_A_BIT (1u << 0)
#define RV32G_CSR_MISA_C_BIT (1u << 2)
#define RV32G_CSR_MISA_D_BIT (1u << 3)
#define RV32G_CSR_MISA_F_BIT (1u << 5)
#define RV32G_CSR_MISA_H_BIT (1u << 7)
#define RV32G_CSR_MISA_I_BIT (1u << 8)
#define RV32G_CSR_MISA_M_BIT (1u << 12)
#define RV32G_CSR_MISA_S_BIT (1u << 18)
#define RV32G_CSR_MISA_U_BIT (1u << 20)
#define RV32G_CSR_MISA_V_BIT (1u << 21)

#define RV32G_CSR_MCOUNTEREN_CY_BIT (1u << 0)
#define RV32G_CSR_MCOUNTEREN_TM_BIT (1u << 1)
#define RV32G_CSR_MCOUNTEREN_IR_BIT (1u << 2)
#define RV32G_CSR_MCOUNTEREN_HPM3_BIT (1u << 3)
#define RV32G_CSR_MCOUNTEREN_HPM4_BIT (1u << 4)
#define RV32G_CSR_MCOUNTEREN_HPM5_BIT (1u << 5)
#define RV32G_CSR_MCOUNTEREN_HPM6_BIT (1u << 6)
#define RV32G_CSR_MCOUNTEREN_HPM7_BIT (1u << 7)
#define RV32G_CSR_MCOUNTEREN_HPM8_BIT (1u << 8)
#define RV32G_CSR_MCOUNTEREN_HPM9_BIT (1u << 9)
#define RV32G_CSR_MCOUNTEREN_HPM10_BIT (1u << 10)
#define RV32G_CSR_MCOUNTEREN_HPM11_BIT (1u << 11)
#define RV32G_CSR_MCOUNTEREN_HPM12_BIT (1u << 12)
#define RV32G_CSR_MCOUNTEREN_HPM13_BIT (1u << 13)
#define RV32G_CSR_MCOUNTEREN_HPM14_BIT (1u << 14)
#define RV32G_CSR_MCOUNTEREN_HPM15_BIT (1u << 15)
#define RV32G_CSR_MCOUNTEREN_HPM16_BIT (1u << 16)
#define RV32G_CSR_MCOUNTEREN_HPM17_BIT (1u << 17)
#define RV32G_CSR_MCOUNTEREN_HPM18_BIT (1u << 18)
#define RV32G_CSR_MCOUNTEREN_HPM19_BIT (1u << 19)
#define RV32G_CSR_MCOUNTEREN_HPM20_BIT (1u << 20)
#define RV32G_CSR_MCOUNTEREN_HPM21_BIT (1u << 21)
#define RV32G_CSR_MCOUNTEREN_HPM22_BIT (1u << 22)
#define RV32G_CSR_MCOUNTEREN_HPM23_BIT (1u << 23)
#define RV32G_CSR_MCOUNTEREN_HPM24_BIT (1u << 24)
#define RV32G_CSR_MCOUNTEREN_HPM25_BIT (1u << 25)
#define RV32G_CSR_MCOUNTEREN_HPM26_BIT (1u << 26)
#define RV32G_CSR_MCOUNTEREN_HPM27_BIT (1u << 27)
#define RV32G_CSR_MCOUNTEREN_HPM28_BIT (1u << 28)
#define RV32G_CSR_MCOUNTEREN_HPM29_BIT (1u << 29)
#define RV32G_CSR_MCOUNTEREN_HPM30_BIT (1u << 30)
#define RV32G_CSR_MCOUNTEREN_HPM31_BIT (1u << 31)

#define RV32G_CSR_MENVCFG_FIOM_BIT (1u << 0)
#define RV32G_CSR_MENVCFG_LPE_BIT (1u << 2)
#define RV32G_CSR_MENVCFG_SSE_BIT (1u << 3)
#define RV32G_CSR_MENVCFG_CBCFE_BIT (1u << 6)
#define RV32G_CSR_MENVCFG_CBZE_BIT (1u << 7)

#define RV32G_CSR_MSTATUSH_SBE_BIT (1u << 4)
#define RV32G_CSR_MSTATUSH_MBE_BIT (1u << 5)
#define RV32G_CSR_MSTATUSH_GVA_BIT (1u << 6)
#define RV32G_CSR_MSTATUSH_MPV_BIT (1u << 7)
#define RV32G_CSR_MSTATUSH_MPELP_BIT (1u << 9)
#define RV32G_CSR_MSTATUSH_MDT_BIT (1u << 10)

#define RV32G_CSR_MENVCFGH_DTE_BIT (1u << 27)
#define RV32G_CSR_MENVCFGH_CDE_BIT (1u << 28)
#define RV32G_CSR_MENVCFGH_ADUE_BIT (1u << 29)
#define RV32G_CSR_MENVCFGH_PBMTE_BIT (1u << 30)
#define RV32G_CSR_MENVCFGH_STCE_BIT (1u << 31)

#define RV32G_CSR_MCOUNTINHIBIT_CY_BIT (1u << 0)
#define RV32G_CSR_MCOUNTINHIBIT_IR_BIT (1u << 2)
#define RV32G_CSR_MCOUNTINHIBIT_HPM3_BIT (1u << 3)
#define RV32G_CSR_MCOUNTINHIBIT_HPM4_BIT (1u << 4)
#define RV32G_CSR_MCOUNTINHIBIT_HPM5_BIT (1u << 5)
#define RV32G_CSR_MCOUNTINHIBIT_HPM6_BIT (1u << 6)
#define RV32G_CSR_MCOUNTINHIBIT_HPM7_BIT (1u << 7)
#define RV32G_CSR_MCOUNTINHIBIT_HPM8_BIT (1u << 8)
#define RV32G_CSR_MCOUNTINHIBIT_HPM9_BIT (1u << 9)
#define RV32G_CSR_MCOUNTINHIBIT_HPM10_BIT (1u << 10)
#define RV32G_CSR_MCOUNTINHIBIT_HPM11_BIT (1u << 11)
#define RV32G_CSR_MCOUNTINHIBIT_HPM12_BIT (1u << 12)
#define RV32G_CSR_MCOUNTINHIBIT_HPM13_BIT (1u << 13)
#define RV32G_CSR_MCOUNTINHIBIT_HPM14_BIT (1u << 14)
#define RV32G_CSR_MCOUNTINHIBIT_HPM15_BIT (1u << 15)
#define RV32G_CSR_MCOUNTINHIBIT_HPM16_BIT (1u << 16)
#define RV32G_CSR_MCOUNTINHIBIT_HPM17_BIT (1u << 17)
#define RV32G_CSR_MCOUNTINHIBIT_HPM18_BIT (1u << 18)
#define RV32G_CSR_MCOUNTINHIBIT_HPM19_BIT (1u << 19)
#define RV32G_CSR_MCOUNTINHIBIT_HPM20_BIT (1u << 20)
#define RV32G_CSR_MCOUNTINHIBIT_HPM21_BIT (1u << 21)
#define RV32G_CSR_MCOUNTINHIBIT_HPM22_BIT (1u << 22)
#define RV32G_CSR_MCOUNTINHIBIT_HPM23_BIT (1u << 23)
#define RV32G_CSR_MCOUNTINHIBIT_HPM24_BIT (1u << 24)
#define RV32G_CSR_MCOUNTINHIBIT_HPM25_BIT (1u << 25)
#define RV32G_CSR_MCOUNTINHIBIT_HPM26_BIT (1u << 26)
#define RV32G_CSR_MCOUNTINHIBIT_HPM27_BIT (1u << 27)
#define RV32G_CSR_MCOUNTINHIBIT_HPM28_BIT (1u << 28)
#define RV32G_CSR_MCOUNTINHIBIT_HPM29_BIT (1u << 29)
#define RV32G_CSR_MCOUNTINHIBIT_HPM30_BIT (1u << 30)
#define RV32G_CSR_MCOUNTINHIBIT_HPM31_BIT (1u << 31)

#define RV32G_CSR_MCAUSE_INTERRUPT_BIT (1u << 31)

#pragma pack(1)
typedef union rv32g_csr_sstatus {
    struct {
        u32 rsvd_0 : 1;
        u32 sie : 1;
        u32 rsvd_1 : 3;
        u32 spie : 1;
        u32 ube : 1;
        u32 rsvd_2 : 1;
        u32 spp : 1;
        u32 vs : 2;
        u32 rsvd_3 : 2;
        u32 fs : 2;
        u32 xs : 2;
        u32 rsvd_4 : 1;
        u32 sum : 1;
        u32 mxr : 1;
        u32 rsvd_5 : 3;
        u32 spelp : 1;
        u32 sdt : 1;
        u32 rsvd_6 : 6;
        u32 sd : 1;
    } reg;
    u32 raw;
} rv32g_csr_sstatus_t;

typedef union rv32g_csr_stvec {
    struct {
        u32 mode : 2;
        u32 base : 30;
    } reg;
    u32 raw;
} rv32g_csr_stvec_t;

typedef union rv32g_csr_scounteren {
    struct {
        u32 cy : 1;
        u32 tm : 1;
        u32 ir : 1;
        u32 hpm3 : 1;
        u32 hpm4 : 1;
        u32 hpm5 : 1;
        u32 hpm6 : 1;
        u32 hpm7 : 1;
        u32 hpm8 : 1;
        u32 hpm9 : 1;
        u32 hpm10 : 1;
        u32 hpm11 : 1;
        u32 hpm12 : 1;
        u32 hpm13 : 1;
        u32 hpm14 : 1;
        u32 hpm15 : 1;
        u32 hpm16 : 1;
        u32 hpm17 : 1;
        u32 hpm18 : 1;
        u32 hpm19 : 1;
        u32 hpm20 : 1;
        u32 hpm21 : 1;
        u32 hpm22 : 1;
        u32 hpm23 : 1;
        u32 hpm24 : 1;
        u32 hpm25 : 1;
        u32 hpm26 : 1;
        u32 hpm27 : 1;
        u32 hpm28 : 1;
        u32 hpm29 : 1;
        u32 hpm30 : 1;
        u32 hpm31 : 1;
    } reg;
    u32 raw;
} rv32g_csr_scounteren_t;

typedef union rv32g_csr_senvcfg {
    struct {
        u32 fiom : 1;
        u32 rsvd_0 : 1;
        u32 lpe : 1;
        u32 sse : 1;
        u32 cbie : 2;
        u32 cbcfe : 1;
        u32 cbze : 1;
        u32 rsvd_1 : 24;
    } reg;
    u32 raw;
} rv32g_csr_senvcfg_t;

typedef union rv32g_csr_scause {
    struct {
        u32 code : 31;
        u32 interrupt : 1;
    } reg;
    u32 raw;
} rv32g_csr_scause_t;

typedef union rv32g_csr_satp {
    struct {
        u32 ppn : 22;
        u32 asid : 9;
        u32 mode : 1;
    } reg;
    u32 raw;
} rv32g_csr_satp_t;

typedef union rv32g_csr_mstatus {
    struct {
        u32 rsvd_0 : 1;
        u32 sie : 1;
        u32 rsvd_1 : 1;
        u32 mie : 1;
        u32 rsvd_2 : 1;
        u32 spie : 1;
        u32 ube : 1;
        u32 mpie : 1;
        u32 spp : 1;
        u32 vs : 2;
        u32 mpp : 2;
        u32 fs : 2;
        u32 xs : 2;
        u32 mprv : 1;
        u32 sum : 1;
        u32 mxr : 1;
        u32 tvm : 1;
        u32 tw : 1;
        u32 tsr : 1;
        u32 spelp : 1;
        u32 std : 1;
        u32 rsvd_3 : 6;
        u32 sd : 1;
    } reg;
    u32 raw;
} rv32g_csr_mstatus_t;

typedef union rv32g_csr_misa {
    struct {
        u32 a : 1;
        u32 rsvd_0 : 1;
        u32 c : 1;
        u32 d : 1;
        u32 rsvd_1 : 1;
        u32 f : 1;
        u32 rsvd_2 : 1;
        u32 h : 1;
        u32 i : 1;
        u32 rsvd_3 : 3;
        u32 m : 1;
        u32 rsvd_4 : 5;
        u32 s : 1;
        u32 rsvd_5 : 1;
        u32 u : 1;
        u32 v : 1;
        u32 rsvd_6 : 10;
    } reg;
    u32 raw;
} rv32g_csr_misa_t;

typedef union rv32g_csr_mtvec {
    struct {
        u32 mode : 2;
        u32 base : 30;
    } reg;
    u32 raw;
} rv32g_csr_mtvec_t;

typedef union rv32g_csr_mcounteren {
    struct {
        u32 cy : 1;
        u32 tm : 1;
        u32 ir : 1;
        u32 hpm3 : 1;
        u32 hpm4 : 1;
        u32 hpm5 : 1;
        u32 hpm6 : 1;
        u32 hpm7 : 1;
        u32 hpm8 : 1;
        u32 hpm9 : 1;
        u32 hpm10 : 1;
        u32 hpm11 : 1;
        u32 hpm12 : 1;
        u32 hpm13 : 1;
        u32 hpm14 : 1;
        u32 hpm15 : 1;
        u32 hpm16 : 1;
        u32 hpm17 : 1;
        u32 hpm18 : 1;
        u32 hpm19 : 1;
        u32 hpm20 : 1;
        u32 hpm21 : 1;
        u32 hpm22 : 1;
        u32 hpm23 : 1;
        u32 hpm24 : 1;
        u32 hpm25 : 1;
        u32 hpm26 : 1;
        u32 hpm27 : 1;
        u32 hpm28 : 1;
        u32 hpm29 : 1;
        u32 hpm30 : 1;
        u32 hpm31 : 1;
    } reg;
    u32 raw;
} rv32g_csr_mcounteren_t;

typedef union rv32g_csr_menvcfg {
    struct {
        u32 fiom : 1;
        u32 rsvd_0 : 1;
        u32 lpe : 1;
        u32 sse : 1;
        u32 cbie : 2;
        u32 cbcfe : 1;
        u32 cbze : 1;
        u32 rsvd_1 : 24;
    } reg;
    u32 raw;
} rv32g_csr_menvcfg_t;

typedef union rv32g_csr_mstatush {
    struct {
        u32 rsvd_0 : 4;
        u32 sbe : 1;
        u32 mbe : 1;
        u32 gva : 1;
        u32 mpv : 1;
        u32 rsvd_1 : 1;
        u32 mpelp : 1;
        u32 mdt : 1;
        u32 rsvd_2 : 21;
    } reg;
    u32 raw;
} rv32g_csr_mstatush_t;

typedef union rv32g_csr_menvcfgh {
    struct {
        u32 pmm : 2;
        u32 rsvd_0 : 25;
        u32 dte : 1;
        u32 cde : 1;
        u32 adue : 1;
        u32 pbmte : 1;
        u32 stce : 1;
    } reg;
    u32 raw;
} rv32g_csr_menvcfgh_t;

typedef union rv32g_csr_mcountinhibit {
    struct {
        u32 cy : 1;
        u32 rsvd_0 : 1;
        u32 ir : 1;
        u32 hpm3 : 1;
        u32 hpm4 : 1;
        u32 hpm5 : 1;
        u32 hpm6 : 1;
        u32 hpm7 : 1;
        u32 hpm8 : 1;
        u32 hpm9 : 1;
        u32 hpm10 : 1;
        u32 hpm11 : 1;
        u32 hpm12 : 1;
        u32 hpm13 : 1;
        u32 hpm14 : 1;
        u32 hpm15 : 1;
        u32 hpm16 : 1;
        u32 hpm17 : 1;
        u32 hpm18 : 1;
        u32 hpm19 : 1;
        u32 hpm20 : 1;
        u32 hpm21 : 1;
        u32 hpm22 : 1;
        u32 hpm23 : 1;
        u32 hpm24 : 1;
        u32 hpm25 : 1;
        u32 hpm26 : 1;
        u32 hpm27 : 1;
        u32 hpm28 : 1;
        u32 hpm29 : 1;
        u32 hpm30 : 1;
        u32 hpm31 : 1;
    } reg;
    u32 raw;
} rv32g_csr_mcountinhibit_t;

typedef union rv32g_csr_mcause {
    struct {
        u32 code : 31;
        u32 interrupt : 1;
    } reg;
    u32 raw;
} rv32g_csr_mcause_t;
#pragma pack()

typedef u32 rv32g_csr_sie_t;
typedef u32 rv32g_csr_sscratch_t;
typedef u32 rv32g_csr_sepc_t;
typedef u32 rv32g_csr_stval_t;
typedef u32 rv32g_csr_sip_t;
typedef u32 rv32g_csr_stimecmp_t;
typedef u32 rv32g_csr_stimecmph_t;
typedef u32 rv32g_csr_medeleg_t;
typedef u32 rv32g_csr_mideleg_t;
typedef u32 rv32g_csr_mie_t;
typedef u32 rv32g_csr_mcyclecfg_t;
typedef u32 rv32g_csr_mhpmevent3_t;
typedef u32 rv32g_csr_mhpmevent4_t;
typedef u32 rv32g_csr_mhpmevent5_t;
typedef u32 rv32g_csr_mhpmevent6_t;
typedef u32 rv32g_csr_mhpmevent7_t;
typedef u32 rv32g_csr_mhpmevent8_t;
typedef u32 rv32g_csr_mhpmevent9_t;
typedef u32 rv32g_csr_mhpmevent10_t;
typedef u32 rv32g_csr_mhpmevent11_t;
typedef u32 rv32g_csr_mhpmevent12_t;
typedef u32 rv32g_csr_mhpmevent13_t;
typedef u32 rv32g_csr_mhpmevent14_t;
typedef u32 rv32g_csr_mhpmevent15_t;
typedef u32 rv32g_csr_mhpmevent16_t;
typedef u32 rv32g_csr_mhpmevent17_t;
typedef u32 rv32g_csr_mhpmevent18_t;
typedef u32 rv32g_csr_mscratch_t;
typedef u32 rv32g_csr_mepc_t;
typedef u32 rv32g_csr_mtval_t;
typedef u32 rv32g_csr_mip_t;
typedef u32 rv32g_csr_mtinst_t;
typedef u32 rv32g_csr_mtval2_t;
typedef u32 rv32g_csr_pmpcfg0_t;
typedef u32 rv32g_csr_pmpcfg1_t;
typedef u32 rv32g_csr_pmpaddr0_t;
typedef u32 rv32g_csr_pmpaddr1_t;
typedef u32 rv32g_csr_pmpaddr2_t;
typedef u32 rv32g_csr_pmpaddr3_t;
typedef u32 rv32g_csr_pmpaddr4_t;
typedef u32 rv32g_csr_pmpaddr5_t;
typedef u32 rv32g_csr_pmpaddr6_t;
typedef u32 rv32g_csr_pmpaddr7_t;
typedef u32 rv32g_csr_pmpaddr8_t;
typedef u32 rv32g_csr_pmpaddr9_t;
typedef u32 rv32g_csr_pmpaddr10_t;
typedef u32 rv32g_csr_pmpaddr11_t;
typedef u32 rv32g_csr_pmpaddr12_t;
typedef u32 rv32g_csr_pmpaddr13_t;
typedef u32 rv32g_csr_pmpaddr14_t;
typedef u32 rv32g_csr_pmpaddr15_t;
typedef u32 rv32g_csr_pmpaddr16_t;
typedef u32 rv32g_csr_tselect_t;
typedef u32 rv32g_csr_tinfo_t;
typedef u32 rv32g_csr_mhpmcounter3_t;
typedef u32 rv32g_csr_mhpmcounter4_t;
typedef u32 rv32g_csr_mhpmcounter5_t;
typedef u32 rv32g_csr_mhpmcounter6_t;
typedef u32 rv32g_csr_mhpmcounter7_t;
typedef u32 rv32g_csr_mhpmcounter8_t;
typedef u32 rv32g_csr_mhpmcounter9_t;
typedef u32 rv32g_csr_mhpmcounter10_t;
typedef u32 rv32g_csr_mhpmcounter11_t;
typedef u32 rv32g_csr_mhpmcounter12_t;
typedef u32 rv32g_csr_mhpmcounter13_t;
typedef u32 rv32g_csr_mhpmcounter14_t;
typedef u32 rv32g_csr_mhpmcounter15_t;
typedef u32 rv32g_csr_mhpmcounter16_t;
typedef u32 rv32g_csr_mhpmcounter17_t;
typedef u32 rv32g_csr_mhpmcounter18_t;
typedef u32 rv32g_csr_mhpmcounter19_t;
typedef u32 rv32g_csr_mhpmcounter20_t;
typedef u32 rv32g_csr_mhpmcounter21_t;
typedef u32 rv32g_csr_mhpmcounter22_t;
typedef u32 rv32g_csr_mhpmcounter23_t;
typedef u32 rv32g_csr_mhpmcounter24_t;
typedef u32 rv32g_csr_mhpmcounter25_t;
typedef u32 rv32g_csr_mhpmcounter26_t;
typedef u32 rv32g_csr_mhpmcounter27_t;
typedef u32 rv32g_csr_mhpmcounter28_t;
typedef u32 rv32g_csr_mhpmcounter29_t;
typedef u32 rv32g_csr_mhpmcounter30_t;
typedef u32 rv32g_csr_mhpmcounter31_t;
typedef u32 rv32g_csr_mhpmcounter3h_t;
typedef u32 rv32g_csr_cycle_t;
typedef u32 rv32g_csr_time_t;
typedef u32 rv32g_csr_instret_t;
typedef u32 rv32g_csr_cycleh_t;
typedef u32 rv32g_csr_timeh_t;
typedef u32 rv32g_csr_scountovf_t;
typedef u32 rv32g_csr_mvendorid_t;
typedef u32 rv32g_csr_marchid_t;
typedef u32 rv32g_csr_mimpid_t;
typedef u32 rv32g_csr_mhartid_t;
typedef u32 rv32g_csr_mtopi_t;

typedef struct csr {
    rv32g_csr_sstatus_t sstatus;
    rv32g_csr_sie_t sie;
    rv32g_csr_stvec_t stvec;
    rv32g_csr_scounteren_t scounteren;
    rv32g_csr_senvcfg_t senvcfg;
    rv32g_csr_sscratch_t sscratch;
    rv32g_csr_sepc_t sepc;
    rv32g_csr_scause_t scause;
    rv32g_csr_stval_t stval;
    rv32g_csr_sip_t sip;
    rv32g_csr_stimecmp_t stimecmp;
    rv32g_csr_stimecmph_t stimecmph;
    rv32g_csr_satp_t satp;
    rv32g_csr_mstatus_t mstatus;
    rv32g_csr_misa_t misa;
    rv32g_csr_medeleg_t medeleg;
    rv32g_csr_mideleg_t mideleg;
    rv32g_csr_mie_t mie;
    rv32g_csr_mtvec_t mtvec;
    rv32g_csr_mcounteren_t mcounteren;
    rv32g_csr_menvcfg_t menvcfg;
    rv32g_csr_mstatush_t mstatush;
    rv32g_csr_menvcfgh_t menvcfgh;
    rv32g_csr_mcountinhibit_t mcountinhibit;
    rv32g_csr_mcyclecfg_t mcyclecfg;
    rv32g_csr_mhpmevent3_t mhpmevent3;
    rv32g_csr_mhpmevent4_t mhpmevent4;
    rv32g_csr_mhpmevent5_t mhpmevent5;
    rv32g_csr_mhpmevent6_t mhpmevent6;
    rv32g_csr_mhpmevent7_t mhpmevent7;
    rv32g_csr_mhpmevent8_t mhpmevent8;
    rv32g_csr_mhpmevent9_t mhpmevent9;
    rv32g_csr_mhpmevent10_t mhpmevent10;
    rv32g_csr_mhpmevent11_t mhpmevent11;
    rv32g_csr_mhpmevent12_t mhpmevent12;
    rv32g_csr_mhpmevent13_t mhpmevent13;
    rv32g_csr_mhpmevent14_t mhpmevent14;
    rv32g_csr_mhpmevent15_t mhpmevent15;
    rv32g_csr_mhpmevent16_t mhpmevent16;
    rv32g_csr_mhpmevent17_t mhpmevent17;
    rv32g_csr_mhpmevent18_t mhpmevent18;
    rv32g_csr_mscratch_t mscratch;
    rv32g_csr_mepc_t mepc;
    rv32g_csr_mcause_t mcause;
    rv32g_csr_mtval_t mtval;
    rv32g_csr_mip_t mip;
    rv32g_csr_mtinst_t mtinst;
    rv32g_csr_mtval2_t mtval2;
    rv32g_csr_pmpcfg0_t pmpcfg0;
    rv32g_csr_pmpcfg1_t pmpcfg1;
    rv32g_csr_pmpaddr0_t pmpaddr0;
    rv32g_csr_pmpaddr1_t pmpaddr1;
    rv32g_csr_pmpaddr2_t pmpaddr2;
    rv32g_csr_pmpaddr3_t pmpaddr3;
    rv32g_csr_pmpaddr4_t pmpaddr4;
    rv32g_csr_pmpaddr5_t pmpaddr5;
    rv32g_csr_pmpaddr6_t pmpaddr6;
    rv32g_csr_pmpaddr7_t pmpaddr7;
    rv32g_csr_pmpaddr8_t pmpaddr8;
    rv32g_csr_pmpaddr9_t pmpaddr9;
    rv32g_csr_pmpaddr10_t pmpaddr10;
    rv32g_csr_pmpaddr11_t pmpaddr11;
    rv32g_csr_pmpaddr12_t pmpaddr12;
    rv32g_csr_pmpaddr13_t pmpaddr13;
    rv32g_csr_pmpaddr14_t pmpaddr14;
    rv32g_csr_pmpaddr15_t pmpaddr15;
    rv32g_csr_pmpaddr16_t pmpaddr16;
    rv32g_csr_tselect_t tselect;
    rv32g_csr_tinfo_t tinfo;
    rv32g_csr_mhpmcounter3_t mhpmcounter3;
    rv32g_csr_mhpmcounter4_t mhpmcounter4;
    rv32g_csr_mhpmcounter5_t mhpmcounter5;
    rv32g_csr_mhpmcounter6_t mhpmcounter6;
    rv32g_csr_mhpmcounter7_t mhpmcounter7;
    rv32g_csr_mhpmcounter8_t mhpmcounter8;
    rv32g_csr_mhpmcounter9_t mhpmcounter9;
    rv32g_csr_mhpmcounter10_t mhpmcounter10;
    rv32g_csr_mhpmcounter11_t mhpmcounter11;
    rv32g_csr_mhpmcounter12_t mhpmcounter12;
    rv32g_csr_mhpmcounter13_t mhpmcounter13;
    rv32g_csr_mhpmcounter14_t mhpmcounter14;
    rv32g_csr_mhpmcounter15_t mhpmcounter15;
    rv32g_csr_mhpmcounter16_t mhpmcounter16;
    rv32g_csr_mhpmcounter17_t mhpmcounter17;
    rv32g_csr_mhpmcounter18_t mhpmcounter18;
    rv32g_csr_mhpmcounter19_t mhpmcounter19;
    rv32g_csr_mhpmcounter20_t mhpmcounter20;
    rv32g_csr_mhpmcounter21_t mhpmcounter21;
    rv32g_csr_mhpmcounter22_t mhpmcounter22;
    rv32g_csr_mhpmcounter23_t mhpmcounter23;
    rv32g_csr_mhpmcounter24_t mhpmcounter24;
    rv32g_csr_mhpmcounter25_t mhpmcounter25;
    rv32g_csr_mhpmcounter26_t mhpmcounter26;
    rv32g_csr_mhpmcounter27_t mhpmcounter27;
    rv32g_csr_mhpmcounter28_t mhpmcounter28;
    rv32g_csr_mhpmcounter29_t mhpmcounter29;
    rv32g_csr_mhpmcounter30_t mhpmcounter30;
    rv32g_csr_mhpmcounter31_t mhpmcounter31;
    rv32g_csr_mhpmcounter3h_t mhpmcounter3h;
    rv32g_csr_cycle_t cycle;
    rv32g_csr_time_t time;
    rv32g_csr_instret_t instret;
    rv32g_csr_cycleh_t cycleh;
    rv32g_csr_timeh_t timeh;
    rv32g_csr_scountovf_t scountovf;
    rv32g_csr_mvendorid_t mvendorid;
    rv32g_csr_marchid_t marchid;
    rv32g_csr_mimpid_t mimpid;
    rv32g_csr_mhartid_t mhartid;
    rv32g_csr_mtopi_t mtopi;
} csr_t;

extern void csr_reset(csr_t *csr);
extern bool csr_read(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data);
extern bool csr_write(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data);
extern const char *csr_name(u32 addr);

#endif