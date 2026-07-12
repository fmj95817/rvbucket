`ifndef HART_SVH
`define HART_SVH

`define MMU_ITLB_SIZE 64
`define MMU_DTLB_SIZE 32

`define BPU_COND_BHT_SIZE 64
`define BPU_JALR_BTB_SIZE 16
`define BPU_JALR_BTB_WAY_NUM 4
`define BPU_RAS_SIZE      16

`define L1_LINE_SIZE 64
`define L1I_SIZE     16384
`define L1I_WAY_NUM  2
`define L1D_SIZE     16384
`define L1D_WAY_NUM  2

`endif
