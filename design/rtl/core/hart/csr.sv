module csr(
    input logic                    clk,
    input logic                    rst_n,
    exu_csr_read_req_if_t.slv      exu_csr_read_req_slv,
    csr_exu_read_rsp_if_t.mst      csr_exu_read_rsp_mst,
    exu_csr_write_req_if_t.slv     exu_csr_write_req_slv,
    csr_exu_write_rsp_if_t.mst     csr_exu_write_rsp_mst,
    core_timer_if_t.slv             core_timer_slv,
    core_m_irq_if_t.slv             core_m_irq_slv,
    ext_irq_if_t.slv                ext_irq_slv,
    trap_csr_write_req_if_t.slv     trap_csr_write_req_slv,
    csr_trap_write_rsp_if_t.mst     csr_trap_write_rsp_mst,
    csr_trap_state_if_t.mst         csr_trap_state_mst,
    csr_mmu_state_if_t.mst          csr_mmu_state_mst,
    csr_lsu_state_if_t.mst          csr_lsu_state_mst
);
    logic [31:0] csr_stvec;
    logic [31:0] csr_scounteren;
    logic [31:0] csr_senvcfg;
    logic [31:0] csr_sscratch;
    logic [31:0] csr_sepc;
    logic [31:0] csr_scause;
    logic [31:0] csr_stval;
    logic [31:0] csr_stimecmp;
    logic [31:0] csr_stimecmph;
    logic [31:0] csr_satp;
    logic [31:0] csr_mstatus;
    logic [31:0] csr_misa;
    logic [31:0] csr_medeleg;
    logic [31:0] csr_mideleg;
    logic [31:0] csr_mie;
    logic [31:0] csr_mtvec;
    logic [31:0] csr_mcounteren;
    logic [31:0] csr_menvcfg;
    logic [31:0] csr_mstatush;
    logic [31:0] csr_menvcfgh;
    logic [31:0] csr_mcountinhibit;
    logic [31:0] csr_mcyclecfg;
    logic [31:0] csr_minstretcfg;
    logic [31:0] csr_mhpmevent3;
    logic [31:0] csr_mhpmevent4;
    logic [31:0] csr_mhpmevent5;
    logic [31:0] csr_mhpmevent6;
    logic [31:0] csr_mhpmevent7;
    logic [31:0] csr_mhpmevent8;
    logic [31:0] csr_mhpmevent9;
    logic [31:0] csr_mhpmevent10;
    logic [31:0] csr_mhpmevent11;
    logic [31:0] csr_mhpmevent12;
    logic [31:0] csr_mhpmevent13;
    logic [31:0] csr_mhpmevent14;
    logic [31:0] csr_mhpmevent15;
    logic [31:0] csr_mhpmevent16;
    logic [31:0] csr_mhpmevent17;
    logic [31:0] csr_mhpmevent18;
    logic [31:0] csr_mhpmevent19;
    logic [31:0] csr_mhpmevent20;
    logic [31:0] csr_mhpmevent21;
    logic [31:0] csr_mhpmevent22;
    logic [31:0] csr_mhpmevent23;
    logic [31:0] csr_mhpmevent24;
    logic [31:0] csr_mhpmevent25;
    logic [31:0] csr_mhpmevent26;
    logic [31:0] csr_mhpmevent27;
    logic [31:0] csr_mhpmevent28;
    logic [31:0] csr_mhpmevent29;
    logic [31:0] csr_mhpmevent30;
    logic [31:0] csr_mhpmevent31;
    logic [31:0] csr_mcyclecfgh;
    logic [31:0] csr_minstretcfgh;
    logic [31:0] csr_mhpmevent3h;
    logic [31:0] csr_mhpmevent4h;
    logic [31:0] csr_mhpmevent5h;
    logic [31:0] csr_mhpmevent6h;
    logic [31:0] csr_mhpmevent7h;
    logic [31:0] csr_mhpmevent8h;
    logic [31:0] csr_mhpmevent9h;
    logic [31:0] csr_mhpmevent10h;
    logic [31:0] csr_mhpmevent11h;
    logic [31:0] csr_mhpmevent12h;
    logic [31:0] csr_mhpmevent13h;
    logic [31:0] csr_mhpmevent14h;
    logic [31:0] csr_mhpmevent15h;
    logic [31:0] csr_mhpmevent16h;
    logic [31:0] csr_mhpmevent17h;
    logic [31:0] csr_mhpmevent18h;
    logic [31:0] csr_mhpmevent19h;
    logic [31:0] csr_mhpmevent20h;
    logic [31:0] csr_mhpmevent21h;
    logic [31:0] csr_mhpmevent22h;
    logic [31:0] csr_mhpmevent23h;
    logic [31:0] csr_mhpmevent24h;
    logic [31:0] csr_mhpmevent25h;
    logic [31:0] csr_mhpmevent26h;
    logic [31:0] csr_mhpmevent27h;
    logic [31:0] csr_mhpmevent28h;
    logic [31:0] csr_mhpmevent29h;
    logic [31:0] csr_mhpmevent30h;
    logic [31:0] csr_mhpmevent31h;
    logic [31:0] csr_mscratch;
    logic [31:0] csr_mepc;
    logic [31:0] csr_mcause;
    logic [31:0] csr_mtval;
    logic [31:0] csr_mip;
    logic [31:0] csr_mtinst;
    logic [31:0] csr_mtval2;
    logic [31:0] csr_pmpcfg0;
    logic [31:0] csr_pmpcfg1;
    logic [31:0] csr_pmpcfg2;
    logic [31:0] csr_pmpcfg3;
    logic [31:0] csr_pmpcfg4;
    logic [31:0] csr_pmpcfg5;
    logic [31:0] csr_pmpcfg6;
    logic [31:0] csr_pmpcfg7;
    logic [31:0] csr_pmpcfg8;
    logic [31:0] csr_pmpcfg9;
    logic [31:0] csr_pmpcfg10;
    logic [31:0] csr_pmpcfg11;
    logic [31:0] csr_pmpcfg12;
    logic [31:0] csr_pmpcfg13;
    logic [31:0] csr_pmpcfg14;
    logic [31:0] csr_pmpcfg15;
    logic [31:0] csr_pmpaddr0;
    logic [31:0] csr_pmpaddr1;
    logic [31:0] csr_pmpaddr2;
    logic [31:0] csr_pmpaddr3;
    logic [31:0] csr_pmpaddr4;
    logic [31:0] csr_pmpaddr5;
    logic [31:0] csr_pmpaddr6;
    logic [31:0] csr_pmpaddr7;
    logic [31:0] csr_pmpaddr8;
    logic [31:0] csr_pmpaddr9;
    logic [31:0] csr_pmpaddr10;
    logic [31:0] csr_pmpaddr11;
    logic [31:0] csr_pmpaddr12;
    logic [31:0] csr_pmpaddr13;
    logic [31:0] csr_pmpaddr14;
    logic [31:0] csr_pmpaddr15;
    logic [31:0] csr_pmpaddr16;
    logic [31:0] csr_pmpaddr17;
    logic [31:0] csr_pmpaddr18;
    logic [31:0] csr_pmpaddr19;
    logic [31:0] csr_pmpaddr20;
    logic [31:0] csr_pmpaddr21;
    logic [31:0] csr_pmpaddr22;
    logic [31:0] csr_pmpaddr23;
    logic [31:0] csr_pmpaddr24;
    logic [31:0] csr_pmpaddr25;
    logic [31:0] csr_pmpaddr26;
    logic [31:0] csr_pmpaddr27;
    logic [31:0] csr_pmpaddr28;
    logic [31:0] csr_pmpaddr29;
    logic [31:0] csr_pmpaddr30;
    logic [31:0] csr_pmpaddr31;
    logic [31:0] csr_pmpaddr32;
    logic [31:0] csr_pmpaddr33;
    logic [31:0] csr_pmpaddr34;
    logic [31:0] csr_pmpaddr35;
    logic [31:0] csr_pmpaddr36;
    logic [31:0] csr_pmpaddr37;
    logic [31:0] csr_pmpaddr38;
    logic [31:0] csr_pmpaddr39;
    logic [31:0] csr_pmpaddr40;
    logic [31:0] csr_pmpaddr41;
    logic [31:0] csr_pmpaddr42;
    logic [31:0] csr_pmpaddr43;
    logic [31:0] csr_pmpaddr44;
    logic [31:0] csr_pmpaddr45;
    logic [31:0] csr_pmpaddr46;
    logic [31:0] csr_pmpaddr47;
    logic [31:0] csr_pmpaddr48;
    logic [31:0] csr_pmpaddr49;
    logic [31:0] csr_pmpaddr50;
    logic [31:0] csr_pmpaddr51;
    logic [31:0] csr_pmpaddr52;
    logic [31:0] csr_pmpaddr53;
    logic [31:0] csr_pmpaddr54;
    logic [31:0] csr_pmpaddr55;
    logic [31:0] csr_pmpaddr56;
    logic [31:0] csr_pmpaddr57;
    logic [31:0] csr_pmpaddr58;
    logic [31:0] csr_pmpaddr59;
    logic [31:0] csr_pmpaddr60;
    logic [31:0] csr_pmpaddr61;
    logic [31:0] csr_pmpaddr62;
    logic [31:0] csr_pmpaddr63;
    logic [31:0] csr_tselect;
    logic [31:0] csr_tinfo;
    logic [31:0] csr_mhpmcounter3;
    logic [31:0] csr_mhpmcounter4;
    logic [31:0] csr_mhpmcounter5;
    logic [31:0] csr_mhpmcounter6;
    logic [31:0] csr_mhpmcounter7;
    logic [31:0] csr_mhpmcounter8;
    logic [31:0] csr_mhpmcounter9;
    logic [31:0] csr_mhpmcounter10;
    logic [31:0] csr_mhpmcounter11;
    logic [31:0] csr_mhpmcounter12;
    logic [31:0] csr_mhpmcounter13;
    logic [31:0] csr_mhpmcounter14;
    logic [31:0] csr_mhpmcounter15;
    logic [31:0] csr_mhpmcounter16;
    logic [31:0] csr_mhpmcounter17;
    logic [31:0] csr_mhpmcounter18;
    logic [31:0] csr_mhpmcounter19;
    logic [31:0] csr_mhpmcounter20;
    logic [31:0] csr_mhpmcounter21;
    logic [31:0] csr_mhpmcounter22;
    logic [31:0] csr_mhpmcounter23;
    logic [31:0] csr_mhpmcounter24;
    logic [31:0] csr_mhpmcounter25;
    logic [31:0] csr_mhpmcounter26;
    logic [31:0] csr_mhpmcounter27;
    logic [31:0] csr_mhpmcounter28;
    logic [31:0] csr_mhpmcounter29;
    logic [31:0] csr_mhpmcounter30;
    logic [31:0] csr_mhpmcounter31;
    logic [31:0] csr_mhpmcounter3h;
    logic [31:0] csr_mhpmcounter4h;
    logic [31:0] csr_mhpmcounter5h;
    logic [31:0] csr_mhpmcounter6h;
    logic [31:0] csr_mhpmcounter7h;
    logic [31:0] csr_mhpmcounter8h;
    logic [31:0] csr_mhpmcounter9h;
    logic [31:0] csr_mhpmcounter10h;
    logic [31:0] csr_mhpmcounter11h;
    logic [31:0] csr_mhpmcounter12h;
    logic [31:0] csr_mhpmcounter13h;
    logic [31:0] csr_mhpmcounter14h;
    logic [31:0] csr_mhpmcounter15h;
    logic [31:0] csr_mhpmcounter16h;
    logic [31:0] csr_mhpmcounter17h;
    logic [31:0] csr_mhpmcounter18h;
    logic [31:0] csr_mhpmcounter19h;
    logic [31:0] csr_mhpmcounter20h;
    logic [31:0] csr_mhpmcounter21h;
    logic [31:0] csr_mhpmcounter22h;
    logic [31:0] csr_mhpmcounter23h;
    logic [31:0] csr_mhpmcounter24h;
    logic [31:0] csr_mhpmcounter25h;
    logic [31:0] csr_mhpmcounter26h;
    logic [31:0] csr_mhpmcounter27h;
    logic [31:0] csr_mhpmcounter28h;
    logic [31:0] csr_mhpmcounter29h;
    logic [31:0] csr_mhpmcounter30h;
    logic [31:0] csr_mhpmcounter31h;
    logic [31:0] csr_cycle;
    logic [31:0] csr_time;
    logic [31:0] csr_instret;
    logic [31:0] csr_cycleh;
    logic [31:0] csr_timeh;
    logic [31:0] csr_scountovf;
    logic [31:0] csr_mvendorid;
    logic [31:0] csr_marchid;
    logic [31:0] csr_mimpid;
    logic [31:0] csr_mhartid;
    logic [31:0] csr_mtopi;

    assign csr_instret = 32'h00000000;
    assign csr_cycleh = 32'h00000000;
    assign csr_scountovf = 32'h00000000;
    assign csr_mvendorid = 32'h00000000;
    assign csr_marchid = 32'h00000000;
    assign csr_mimpid = 32'h00000000;
    assign csr_mhartid = 32'h00000000;
    assign csr_mtopi = 32'h00000000;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            csr_stvec <= 32'h00000000;
            csr_scounteren <= 32'h00000000;
            csr_senvcfg <= 32'h00000000;
            csr_sscratch <= 32'h00000000;
            csr_sepc <= 32'h00000000;
            csr_scause <= 32'h00000000;
            csr_stval <= 32'h00000000;
            csr_stimecmp <= 32'hffffffff;
            csr_stimecmph <= 32'hffffffff;
            csr_satp <= 32'h00000000;
            csr_mstatus <= 32'h00000000;
            csr_misa <= 32'h40141101;
            csr_medeleg <= 32'h00000000;
            csr_mideleg <= 32'h00000000;
            csr_mie <= 32'h00000000;
            csr_mtvec <= 32'h00000000;
            csr_mcounteren <= 32'h00000000;
            csr_menvcfg <= 32'h00000000;
            csr_mstatush <= 32'h00000000;
            csr_menvcfgh <= 32'h00000000;
            csr_mcountinhibit <= 32'h00000000;
            csr_mcyclecfg <= 32'h00000000;
            csr_minstretcfg <= 32'h00000000;
            csr_mhpmevent3 <= 32'h00000000;
            csr_mhpmevent4 <= 32'h00000000;
            csr_mhpmevent5 <= 32'h00000000;
            csr_mhpmevent6 <= 32'h00000000;
            csr_mhpmevent7 <= 32'h00000000;
            csr_mhpmevent8 <= 32'h00000000;
            csr_mhpmevent9 <= 32'h00000000;
            csr_mhpmevent10 <= 32'h00000000;
            csr_mhpmevent11 <= 32'h00000000;
            csr_mhpmevent12 <= 32'h00000000;
            csr_mhpmevent13 <= 32'h00000000;
            csr_mhpmevent14 <= 32'h00000000;
            csr_mhpmevent15 <= 32'h00000000;
            csr_mhpmevent16 <= 32'h00000000;
            csr_mhpmevent17 <= 32'h00000000;
            csr_mhpmevent18 <= 32'h00000000;
            csr_mhpmevent19 <= 32'h00000000;
            csr_mhpmevent20 <= 32'h00000000;
            csr_mhpmevent21 <= 32'h00000000;
            csr_mhpmevent22 <= 32'h00000000;
            csr_mhpmevent23 <= 32'h00000000;
            csr_mhpmevent24 <= 32'h00000000;
            csr_mhpmevent25 <= 32'h00000000;
            csr_mhpmevent26 <= 32'h00000000;
            csr_mhpmevent27 <= 32'h00000000;
            csr_mhpmevent28 <= 32'h00000000;
            csr_mhpmevent29 <= 32'h00000000;
            csr_mhpmevent30 <= 32'h00000000;
            csr_mhpmevent31 <= 32'h00000000;
            csr_mcyclecfgh <= 32'h00000000;
            csr_minstretcfgh <= 32'h00000000;
            csr_mhpmevent3h <= 32'h00000000;
            csr_mhpmevent4h <= 32'h00000000;
            csr_mhpmevent5h <= 32'h00000000;
            csr_mhpmevent6h <= 32'h00000000;
            csr_mhpmevent7h <= 32'h00000000;
            csr_mhpmevent8h <= 32'h00000000;
            csr_mhpmevent9h <= 32'h00000000;
            csr_mhpmevent10h <= 32'h00000000;
            csr_mhpmevent11h <= 32'h00000000;
            csr_mhpmevent12h <= 32'h00000000;
            csr_mhpmevent13h <= 32'h00000000;
            csr_mhpmevent14h <= 32'h00000000;
            csr_mhpmevent15h <= 32'h00000000;
            csr_mhpmevent16h <= 32'h00000000;
            csr_mhpmevent17h <= 32'h00000000;
            csr_mhpmevent18h <= 32'h00000000;
            csr_mhpmevent19h <= 32'h00000000;
            csr_mhpmevent20h <= 32'h00000000;
            csr_mhpmevent21h <= 32'h00000000;
            csr_mhpmevent22h <= 32'h00000000;
            csr_mhpmevent23h <= 32'h00000000;
            csr_mhpmevent24h <= 32'h00000000;
            csr_mhpmevent25h <= 32'h00000000;
            csr_mhpmevent26h <= 32'h00000000;
            csr_mhpmevent27h <= 32'h00000000;
            csr_mhpmevent28h <= 32'h00000000;
            csr_mhpmevent29h <= 32'h00000000;
            csr_mhpmevent30h <= 32'h00000000;
            csr_mhpmevent31h <= 32'h00000000;
            csr_mscratch <= 32'h00000000;
            csr_mepc <= 32'h00000000;
            csr_mcause <= 32'h00000000;
            csr_mtval <= 32'h00000000;
            csr_mip <= 32'h00000000;
            csr_mtinst <= 32'h00000000;
            csr_mtval2 <= 32'h00000000;
            csr_pmpcfg0 <= 32'h00000000;
            csr_pmpcfg1 <= 32'h00000000;
            csr_pmpcfg2 <= 32'h00000000;
            csr_pmpcfg3 <= 32'h00000000;
            csr_pmpcfg4 <= 32'h00000000;
            csr_pmpcfg5 <= 32'h00000000;
            csr_pmpcfg6 <= 32'h00000000;
            csr_pmpcfg7 <= 32'h00000000;
            csr_pmpcfg8 <= 32'h00000000;
            csr_pmpcfg9 <= 32'h00000000;
            csr_pmpcfg10 <= 32'h00000000;
            csr_pmpcfg11 <= 32'h00000000;
            csr_pmpcfg12 <= 32'h00000000;
            csr_pmpcfg13 <= 32'h00000000;
            csr_pmpcfg14 <= 32'h00000000;
            csr_pmpcfg15 <= 32'h00000000;
            csr_pmpaddr0 <= 32'h00000000;
            csr_pmpaddr1 <= 32'h00000000;
            csr_pmpaddr2 <= 32'h00000000;
            csr_pmpaddr3 <= 32'h00000000;
            csr_pmpaddr4 <= 32'h00000000;
            csr_pmpaddr5 <= 32'h00000000;
            csr_pmpaddr6 <= 32'h00000000;
            csr_pmpaddr7 <= 32'h00000000;
            csr_pmpaddr8 <= 32'h00000000;
            csr_pmpaddr9 <= 32'h00000000;
            csr_pmpaddr10 <= 32'h00000000;
            csr_pmpaddr11 <= 32'h00000000;
            csr_pmpaddr12 <= 32'h00000000;
            csr_pmpaddr13 <= 32'h00000000;
            csr_pmpaddr14 <= 32'h00000000;
            csr_pmpaddr15 <= 32'h00000000;
            csr_pmpaddr16 <= 32'h00000000;
            csr_pmpaddr17 <= 32'h00000000;
            csr_pmpaddr18 <= 32'h00000000;
            csr_pmpaddr19 <= 32'h00000000;
            csr_pmpaddr20 <= 32'h00000000;
            csr_pmpaddr21 <= 32'h00000000;
            csr_pmpaddr22 <= 32'h00000000;
            csr_pmpaddr23 <= 32'h00000000;
            csr_pmpaddr24 <= 32'h00000000;
            csr_pmpaddr25 <= 32'h00000000;
            csr_pmpaddr26 <= 32'h00000000;
            csr_pmpaddr27 <= 32'h00000000;
            csr_pmpaddr28 <= 32'h00000000;
            csr_pmpaddr29 <= 32'h00000000;
            csr_pmpaddr30 <= 32'h00000000;
            csr_pmpaddr31 <= 32'h00000000;
            csr_pmpaddr32 <= 32'h00000000;
            csr_pmpaddr33 <= 32'h00000000;
            csr_pmpaddr34 <= 32'h00000000;
            csr_pmpaddr35 <= 32'h00000000;
            csr_pmpaddr36 <= 32'h00000000;
            csr_pmpaddr37 <= 32'h00000000;
            csr_pmpaddr38 <= 32'h00000000;
            csr_pmpaddr39 <= 32'h00000000;
            csr_pmpaddr40 <= 32'h00000000;
            csr_pmpaddr41 <= 32'h00000000;
            csr_pmpaddr42 <= 32'h00000000;
            csr_pmpaddr43 <= 32'h00000000;
            csr_pmpaddr44 <= 32'h00000000;
            csr_pmpaddr45 <= 32'h00000000;
            csr_pmpaddr46 <= 32'h00000000;
            csr_pmpaddr47 <= 32'h00000000;
            csr_pmpaddr48 <= 32'h00000000;
            csr_pmpaddr49 <= 32'h00000000;
            csr_pmpaddr50 <= 32'h00000000;
            csr_pmpaddr51 <= 32'h00000000;
            csr_pmpaddr52 <= 32'h00000000;
            csr_pmpaddr53 <= 32'h00000000;
            csr_pmpaddr54 <= 32'h00000000;
            csr_pmpaddr55 <= 32'h00000000;
            csr_pmpaddr56 <= 32'h00000000;
            csr_pmpaddr57 <= 32'h00000000;
            csr_pmpaddr58 <= 32'h00000000;
            csr_pmpaddr59 <= 32'h00000000;
            csr_pmpaddr60 <= 32'h00000000;
            csr_pmpaddr61 <= 32'h00000000;
            csr_pmpaddr62 <= 32'h00000000;
            csr_pmpaddr63 <= 32'h00000000;
            csr_tselect <= 32'h00000000;
            csr_tinfo <= 32'h00000000;
            csr_mhpmcounter3 <= 32'h00000000;
            csr_mhpmcounter4 <= 32'h00000000;
            csr_mhpmcounter5 <= 32'h00000000;
            csr_mhpmcounter6 <= 32'h00000000;
            csr_mhpmcounter7 <= 32'h00000000;
            csr_mhpmcounter8 <= 32'h00000000;
            csr_mhpmcounter9 <= 32'h00000000;
            csr_mhpmcounter10 <= 32'h00000000;
            csr_mhpmcounter11 <= 32'h00000000;
            csr_mhpmcounter12 <= 32'h00000000;
            csr_mhpmcounter13 <= 32'h00000000;
            csr_mhpmcounter14 <= 32'h00000000;
            csr_mhpmcounter15 <= 32'h00000000;
            csr_mhpmcounter16 <= 32'h00000000;
            csr_mhpmcounter17 <= 32'h00000000;
            csr_mhpmcounter18 <= 32'h00000000;
            csr_mhpmcounter19 <= 32'h00000000;
            csr_mhpmcounter20 <= 32'h00000000;
            csr_mhpmcounter21 <= 32'h00000000;
            csr_mhpmcounter22 <= 32'h00000000;
            csr_mhpmcounter23 <= 32'h00000000;
            csr_mhpmcounter24 <= 32'h00000000;
            csr_mhpmcounter25 <= 32'h00000000;
            csr_mhpmcounter26 <= 32'h00000000;
            csr_mhpmcounter27 <= 32'h00000000;
            csr_mhpmcounter28 <= 32'h00000000;
            csr_mhpmcounter29 <= 32'h00000000;
            csr_mhpmcounter30 <= 32'h00000000;
            csr_mhpmcounter31 <= 32'h00000000;
            csr_mhpmcounter3h <= 32'h00000000;
            csr_mhpmcounter4h <= 32'h00000000;
            csr_mhpmcounter5h <= 32'h00000000;
            csr_mhpmcounter6h <= 32'h00000000;
            csr_mhpmcounter7h <= 32'h00000000;
            csr_mhpmcounter8h <= 32'h00000000;
            csr_mhpmcounter9h <= 32'h00000000;
            csr_mhpmcounter10h <= 32'h00000000;
            csr_mhpmcounter11h <= 32'h00000000;
            csr_mhpmcounter12h <= 32'h00000000;
            csr_mhpmcounter13h <= 32'h00000000;
            csr_mhpmcounter14h <= 32'h00000000;
            csr_mhpmcounter15h <= 32'h00000000;
            csr_mhpmcounter16h <= 32'h00000000;
            csr_mhpmcounter17h <= 32'h00000000;
            csr_mhpmcounter18h <= 32'h00000000;
            csr_mhpmcounter19h <= 32'h00000000;
            csr_mhpmcounter20h <= 32'h00000000;
            csr_mhpmcounter21h <= 32'h00000000;
            csr_mhpmcounter22h <= 32'h00000000;
            csr_mhpmcounter23h <= 32'h00000000;
            csr_mhpmcounter24h <= 32'h00000000;
            csr_mhpmcounter25h <= 32'h00000000;
            csr_mhpmcounter26h <= 32'h00000000;
            csr_mhpmcounter27h <= 32'h00000000;
            csr_mhpmcounter28h <= 32'h00000000;
            csr_mhpmcounter29h <= 32'h00000000;
            csr_mhpmcounter30h <= 32'h00000000;
            csr_mhpmcounter31h <= 32'h00000000;
            csr_cycle <= 32'h00000000;
            csr_time <= 32'h00000000;
            csr_timeh <= 32'h00000000;
        end else begin
            csr_cycle <= csr_cycle + 1'b1;
            csr_time <= core_timer_slv.pkt.mtime[31:0];
            csr_timeh <= core_timer_slv.pkt.mtime[63:32];
            csr_mip[7] <= core_m_irq_slv.pkt.mtimer;
            csr_mip[3] <= core_m_irq_slv.pkt.msw;
            csr_mip[5] <= core_timer_slv.pkt.mtime >= {csr_stimecmph, csr_stimecmp};
            csr_mip[11] <= ext_irq_slv.pkt.irq;
            csr_mip[9] <= ext_irq_slv.pkt.irq;
            if (trap_csr_write_req_slv.vld && csr_trap_write_rsp_mst.pkt.ok) begin
                case (trap_csr_write_req_slv.pkt.addr)
                    12'h100: csr_mstatus <= (csr_mstatus & ~32'h800de762) | (trap_csr_write_req_slv.pkt.val & 32'h800de762);
                    12'h104: csr_mie <= (csr_mie & ~32'h00002222) | (trap_csr_write_req_slv.pkt.val & 32'h00002222);
                    12'h105: csr_stvec <= trap_csr_write_req_slv.pkt.val;
                    12'h106: csr_scounteren <= trap_csr_write_req_slv.pkt.val;
                    12'h10a: csr_senvcfg <= trap_csr_write_req_slv.pkt.val;
                    12'h140: csr_sscratch <= trap_csr_write_req_slv.pkt.val;
                    12'h141: csr_sepc <= trap_csr_write_req_slv.pkt.val;
                    12'h142: csr_scause <= trap_csr_write_req_slv.pkt.val;
                    12'h143: csr_stval <= trap_csr_write_req_slv.pkt.val;
                    12'h144: csr_mip <= (csr_mip & ~32'h00002222) | (trap_csr_write_req_slv.pkt.val & 32'h00002222);
                    12'h14d: csr_stimecmp <= trap_csr_write_req_slv.pkt.val;
                    12'h15d: csr_stimecmph <= trap_csr_write_req_slv.pkt.val;
                    12'h180: csr_satp <= trap_csr_write_req_slv.pkt.val;
                    12'h300: csr_mstatus <= trap_csr_write_req_slv.pkt.val;
                    12'h301: csr_misa <= trap_csr_write_req_slv.pkt.val;
                    12'h302: csr_medeleg <= trap_csr_write_req_slv.pkt.val;
                    12'h303: csr_mideleg <= trap_csr_write_req_slv.pkt.val;
                    12'h304: csr_mie <= trap_csr_write_req_slv.pkt.val;
                    12'h305: csr_mtvec <= trap_csr_write_req_slv.pkt.val;
                    12'h306: csr_mcounteren <= trap_csr_write_req_slv.pkt.val;
                    12'h30a: csr_menvcfg <= trap_csr_write_req_slv.pkt.val;
                    12'h310: csr_mstatush <= trap_csr_write_req_slv.pkt.val;
                    12'h31a: csr_menvcfgh <= trap_csr_write_req_slv.pkt.val;
                    12'h320: csr_mcountinhibit <= trap_csr_write_req_slv.pkt.val;
                    12'h321: csr_mcyclecfg <= trap_csr_write_req_slv.pkt.val;
                    12'h322: csr_minstretcfg <= trap_csr_write_req_slv.pkt.val;
                    12'h323: csr_mhpmevent3 <= trap_csr_write_req_slv.pkt.val;
                    12'h324: csr_mhpmevent4 <= trap_csr_write_req_slv.pkt.val;
                    12'h325: csr_mhpmevent5 <= trap_csr_write_req_slv.pkt.val;
                    12'h326: csr_mhpmevent6 <= trap_csr_write_req_slv.pkt.val;
                    12'h327: csr_mhpmevent7 <= trap_csr_write_req_slv.pkt.val;
                    12'h328: csr_mhpmevent8 <= trap_csr_write_req_slv.pkt.val;
                    12'h329: csr_mhpmevent9 <= trap_csr_write_req_slv.pkt.val;
                    12'h32a: csr_mhpmevent10 <= trap_csr_write_req_slv.pkt.val;
                    12'h32b: csr_mhpmevent11 <= trap_csr_write_req_slv.pkt.val;
                    12'h32c: csr_mhpmevent12 <= trap_csr_write_req_slv.pkt.val;
                    12'h32d: csr_mhpmevent13 <= trap_csr_write_req_slv.pkt.val;
                    12'h32e: csr_mhpmevent14 <= trap_csr_write_req_slv.pkt.val;
                    12'h32f: csr_mhpmevent15 <= trap_csr_write_req_slv.pkt.val;
                    12'h330: csr_mhpmevent16 <= trap_csr_write_req_slv.pkt.val;
                    12'h331: csr_mhpmevent17 <= trap_csr_write_req_slv.pkt.val;
                    12'h332: csr_mhpmevent18 <= trap_csr_write_req_slv.pkt.val;
                    12'h333: csr_mhpmevent19 <= trap_csr_write_req_slv.pkt.val;
                    12'h334: csr_mhpmevent20 <= trap_csr_write_req_slv.pkt.val;
                    12'h335: csr_mhpmevent21 <= trap_csr_write_req_slv.pkt.val;
                    12'h336: csr_mhpmevent22 <= trap_csr_write_req_slv.pkt.val;
                    12'h337: csr_mhpmevent23 <= trap_csr_write_req_slv.pkt.val;
                    12'h338: csr_mhpmevent24 <= trap_csr_write_req_slv.pkt.val;
                    12'h339: csr_mhpmevent25 <= trap_csr_write_req_slv.pkt.val;
                    12'h33a: csr_mhpmevent26 <= trap_csr_write_req_slv.pkt.val;
                    12'h33b: csr_mhpmevent27 <= trap_csr_write_req_slv.pkt.val;
                    12'h33c: csr_mhpmevent28 <= trap_csr_write_req_slv.pkt.val;
                    12'h33d: csr_mhpmevent29 <= trap_csr_write_req_slv.pkt.val;
                    12'h33e: csr_mhpmevent30 <= trap_csr_write_req_slv.pkt.val;
                    12'h33f: csr_mhpmevent31 <= trap_csr_write_req_slv.pkt.val;
                    12'h721: csr_mcyclecfgh <= trap_csr_write_req_slv.pkt.val;
                    12'h722: csr_minstretcfgh <= trap_csr_write_req_slv.pkt.val;
                    12'h723: csr_mhpmevent3h <= trap_csr_write_req_slv.pkt.val;
                    12'h724: csr_mhpmevent4h <= trap_csr_write_req_slv.pkt.val;
                    12'h725: csr_mhpmevent5h <= trap_csr_write_req_slv.pkt.val;
                    12'h726: csr_mhpmevent6h <= trap_csr_write_req_slv.pkt.val;
                    12'h727: csr_mhpmevent7h <= trap_csr_write_req_slv.pkt.val;
                    12'h728: csr_mhpmevent8h <= trap_csr_write_req_slv.pkt.val;
                    12'h729: csr_mhpmevent9h <= trap_csr_write_req_slv.pkt.val;
                    12'h72a: csr_mhpmevent10h <= trap_csr_write_req_slv.pkt.val;
                    12'h72b: csr_mhpmevent11h <= trap_csr_write_req_slv.pkt.val;
                    12'h72c: csr_mhpmevent12h <= trap_csr_write_req_slv.pkt.val;
                    12'h72d: csr_mhpmevent13h <= trap_csr_write_req_slv.pkt.val;
                    12'h72e: csr_mhpmevent14h <= trap_csr_write_req_slv.pkt.val;
                    12'h72f: csr_mhpmevent15h <= trap_csr_write_req_slv.pkt.val;
                    12'h730: csr_mhpmevent16h <= trap_csr_write_req_slv.pkt.val;
                    12'h731: csr_mhpmevent17h <= trap_csr_write_req_slv.pkt.val;
                    12'h732: csr_mhpmevent18h <= trap_csr_write_req_slv.pkt.val;
                    12'h733: csr_mhpmevent19h <= trap_csr_write_req_slv.pkt.val;
                    12'h734: csr_mhpmevent20h <= trap_csr_write_req_slv.pkt.val;
                    12'h735: csr_mhpmevent21h <= trap_csr_write_req_slv.pkt.val;
                    12'h736: csr_mhpmevent22h <= trap_csr_write_req_slv.pkt.val;
                    12'h737: csr_mhpmevent23h <= trap_csr_write_req_slv.pkt.val;
                    12'h738: csr_mhpmevent24h <= trap_csr_write_req_slv.pkt.val;
                    12'h739: csr_mhpmevent25h <= trap_csr_write_req_slv.pkt.val;
                    12'h73a: csr_mhpmevent26h <= trap_csr_write_req_slv.pkt.val;
                    12'h73b: csr_mhpmevent27h <= trap_csr_write_req_slv.pkt.val;
                    12'h73c: csr_mhpmevent28h <= trap_csr_write_req_slv.pkt.val;
                    12'h73d: csr_mhpmevent29h <= trap_csr_write_req_slv.pkt.val;
                    12'h73e: csr_mhpmevent30h <= trap_csr_write_req_slv.pkt.val;
                    12'h73f: csr_mhpmevent31h <= trap_csr_write_req_slv.pkt.val;
                    12'h340: csr_mscratch <= trap_csr_write_req_slv.pkt.val;
                    12'h341: csr_mepc <= trap_csr_write_req_slv.pkt.val;
                    12'h342: csr_mcause <= trap_csr_write_req_slv.pkt.val;
                    12'h343: csr_mtval <= trap_csr_write_req_slv.pkt.val;
                    12'h344: csr_mip <= trap_csr_write_req_slv.pkt.val;
                    12'h34a: csr_mtinst <= trap_csr_write_req_slv.pkt.val;
                    12'h34b: csr_mtval2 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a0: csr_pmpcfg0 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a1: csr_pmpcfg1 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a2: csr_pmpcfg2 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a3: csr_pmpcfg3 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a4: csr_pmpcfg4 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a5: csr_pmpcfg5 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a6: csr_pmpcfg6 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a7: csr_pmpcfg7 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a8: csr_pmpcfg8 <= trap_csr_write_req_slv.pkt.val;
                    12'h3a9: csr_pmpcfg9 <= trap_csr_write_req_slv.pkt.val;
                    12'h3aa: csr_pmpcfg10 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ab: csr_pmpcfg11 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ac: csr_pmpcfg12 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ad: csr_pmpcfg13 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ae: csr_pmpcfg14 <= trap_csr_write_req_slv.pkt.val;
                    12'h3af: csr_pmpcfg15 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b0: csr_pmpaddr0 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b1: csr_pmpaddr1 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b2: csr_pmpaddr2 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b3: csr_pmpaddr3 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b4: csr_pmpaddr4 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b5: csr_pmpaddr5 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b6: csr_pmpaddr6 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b7: csr_pmpaddr7 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b8: csr_pmpaddr8 <= trap_csr_write_req_slv.pkt.val;
                    12'h3b9: csr_pmpaddr9 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ba: csr_pmpaddr10 <= trap_csr_write_req_slv.pkt.val;
                    12'h3bb: csr_pmpaddr11 <= trap_csr_write_req_slv.pkt.val;
                    12'h3bc: csr_pmpaddr12 <= trap_csr_write_req_slv.pkt.val;
                    12'h3bd: csr_pmpaddr13 <= trap_csr_write_req_slv.pkt.val;
                    12'h3be: csr_pmpaddr14 <= trap_csr_write_req_slv.pkt.val;
                    12'h3bf: csr_pmpaddr15 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c0: csr_pmpaddr16 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c1: csr_pmpaddr17 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c2: csr_pmpaddr18 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c3: csr_pmpaddr19 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c4: csr_pmpaddr20 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c5: csr_pmpaddr21 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c6: csr_pmpaddr22 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c7: csr_pmpaddr23 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c8: csr_pmpaddr24 <= trap_csr_write_req_slv.pkt.val;
                    12'h3c9: csr_pmpaddr25 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ca: csr_pmpaddr26 <= trap_csr_write_req_slv.pkt.val;
                    12'h3cb: csr_pmpaddr27 <= trap_csr_write_req_slv.pkt.val;
                    12'h3cc: csr_pmpaddr28 <= trap_csr_write_req_slv.pkt.val;
                    12'h3cd: csr_pmpaddr29 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ce: csr_pmpaddr30 <= trap_csr_write_req_slv.pkt.val;
                    12'h3cf: csr_pmpaddr31 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d0: csr_pmpaddr32 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d1: csr_pmpaddr33 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d2: csr_pmpaddr34 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d3: csr_pmpaddr35 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d4: csr_pmpaddr36 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d5: csr_pmpaddr37 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d6: csr_pmpaddr38 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d7: csr_pmpaddr39 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d8: csr_pmpaddr40 <= trap_csr_write_req_slv.pkt.val;
                    12'h3d9: csr_pmpaddr41 <= trap_csr_write_req_slv.pkt.val;
                    12'h3da: csr_pmpaddr42 <= trap_csr_write_req_slv.pkt.val;
                    12'h3db: csr_pmpaddr43 <= trap_csr_write_req_slv.pkt.val;
                    12'h3dc: csr_pmpaddr44 <= trap_csr_write_req_slv.pkt.val;
                    12'h3dd: csr_pmpaddr45 <= trap_csr_write_req_slv.pkt.val;
                    12'h3de: csr_pmpaddr46 <= trap_csr_write_req_slv.pkt.val;
                    12'h3df: csr_pmpaddr47 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e0: csr_pmpaddr48 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e1: csr_pmpaddr49 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e2: csr_pmpaddr50 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e3: csr_pmpaddr51 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e4: csr_pmpaddr52 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e5: csr_pmpaddr53 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e6: csr_pmpaddr54 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e7: csr_pmpaddr55 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e8: csr_pmpaddr56 <= trap_csr_write_req_slv.pkt.val;
                    12'h3e9: csr_pmpaddr57 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ea: csr_pmpaddr58 <= trap_csr_write_req_slv.pkt.val;
                    12'h3eb: csr_pmpaddr59 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ec: csr_pmpaddr60 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ed: csr_pmpaddr61 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ee: csr_pmpaddr62 <= trap_csr_write_req_slv.pkt.val;
                    12'h3ef: csr_pmpaddr63 <= trap_csr_write_req_slv.pkt.val;
                    12'h7a0: csr_tselect <= trap_csr_write_req_slv.pkt.val;
                    12'h7a4: csr_tinfo <= trap_csr_write_req_slv.pkt.val;
                    12'hb03: csr_mhpmcounter3 <= trap_csr_write_req_slv.pkt.val;
                    12'hb04: csr_mhpmcounter4 <= trap_csr_write_req_slv.pkt.val;
                    12'hb05: csr_mhpmcounter5 <= trap_csr_write_req_slv.pkt.val;
                    12'hb06: csr_mhpmcounter6 <= trap_csr_write_req_slv.pkt.val;
                    12'hb07: csr_mhpmcounter7 <= trap_csr_write_req_slv.pkt.val;
                    12'hb08: csr_mhpmcounter8 <= trap_csr_write_req_slv.pkt.val;
                    12'hb09: csr_mhpmcounter9 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0a: csr_mhpmcounter10 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0b: csr_mhpmcounter11 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0c: csr_mhpmcounter12 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0d: csr_mhpmcounter13 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0e: csr_mhpmcounter14 <= trap_csr_write_req_slv.pkt.val;
                    12'hb0f: csr_mhpmcounter15 <= trap_csr_write_req_slv.pkt.val;
                    12'hb10: csr_mhpmcounter16 <= trap_csr_write_req_slv.pkt.val;
                    12'hb11: csr_mhpmcounter17 <= trap_csr_write_req_slv.pkt.val;
                    12'hb12: csr_mhpmcounter18 <= trap_csr_write_req_slv.pkt.val;
                    12'hb13: csr_mhpmcounter19 <= trap_csr_write_req_slv.pkt.val;
                    12'hb14: csr_mhpmcounter20 <= trap_csr_write_req_slv.pkt.val;
                    12'hb15: csr_mhpmcounter21 <= trap_csr_write_req_slv.pkt.val;
                    12'hb16: csr_mhpmcounter22 <= trap_csr_write_req_slv.pkt.val;
                    12'hb17: csr_mhpmcounter23 <= trap_csr_write_req_slv.pkt.val;
                    12'hb18: csr_mhpmcounter24 <= trap_csr_write_req_slv.pkt.val;
                    12'hb19: csr_mhpmcounter25 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1a: csr_mhpmcounter26 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1b: csr_mhpmcounter27 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1c: csr_mhpmcounter28 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1d: csr_mhpmcounter29 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1e: csr_mhpmcounter30 <= trap_csr_write_req_slv.pkt.val;
                    12'hb1f: csr_mhpmcounter31 <= trap_csr_write_req_slv.pkt.val;
                    12'hb83: csr_mhpmcounter3h <= trap_csr_write_req_slv.pkt.val;
                    12'hb84: csr_mhpmcounter4h <= trap_csr_write_req_slv.pkt.val;
                    12'hb85: csr_mhpmcounter5h <= trap_csr_write_req_slv.pkt.val;
                    12'hb86: csr_mhpmcounter6h <= trap_csr_write_req_slv.pkt.val;
                    12'hb87: csr_mhpmcounter7h <= trap_csr_write_req_slv.pkt.val;
                    12'hb88: csr_mhpmcounter8h <= trap_csr_write_req_slv.pkt.val;
                    12'hb89: csr_mhpmcounter9h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8a: csr_mhpmcounter10h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8b: csr_mhpmcounter11h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8c: csr_mhpmcounter12h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8d: csr_mhpmcounter13h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8e: csr_mhpmcounter14h <= trap_csr_write_req_slv.pkt.val;
                    12'hb8f: csr_mhpmcounter15h <= trap_csr_write_req_slv.pkt.val;
                    12'hb90: csr_mhpmcounter16h <= trap_csr_write_req_slv.pkt.val;
                    12'hb91: csr_mhpmcounter17h <= trap_csr_write_req_slv.pkt.val;
                    12'hb92: csr_mhpmcounter18h <= trap_csr_write_req_slv.pkt.val;
                    12'hb93: csr_mhpmcounter19h <= trap_csr_write_req_slv.pkt.val;
                    12'hb94: csr_mhpmcounter20h <= trap_csr_write_req_slv.pkt.val;
                    12'hb95: csr_mhpmcounter21h <= trap_csr_write_req_slv.pkt.val;
                    12'hb96: csr_mhpmcounter22h <= trap_csr_write_req_slv.pkt.val;
                    12'hb97: csr_mhpmcounter23h <= trap_csr_write_req_slv.pkt.val;
                    12'hb98: csr_mhpmcounter24h <= trap_csr_write_req_slv.pkt.val;
                    12'hb99: csr_mhpmcounter25h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9a: csr_mhpmcounter26h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9b: csr_mhpmcounter27h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9c: csr_mhpmcounter28h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9d: csr_mhpmcounter29h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9e: csr_mhpmcounter30h <= trap_csr_write_req_slv.pkt.val;
                    12'hb9f: csr_mhpmcounter31h <= trap_csr_write_req_slv.pkt.val;
                    default: ;
                endcase
            end else if (exu_csr_write_req_slv.vld && csr_exu_write_rsp_mst.pkt.ok) begin
                case (exu_csr_write_req_slv.pkt.addr)
                    12'h100: csr_mstatus <= (csr_mstatus & ~32'h800de762) | (exu_csr_write_req_slv.pkt.val & 32'h800de762);
                    12'h104: csr_mie <= (csr_mie & ~32'h00002222) | (exu_csr_write_req_slv.pkt.val & 32'h00002222);
                    12'h105: csr_stvec <= exu_csr_write_req_slv.pkt.val;
                    12'h106: csr_scounteren <= exu_csr_write_req_slv.pkt.val;
                    12'h10a: csr_senvcfg <= exu_csr_write_req_slv.pkt.val;
                    12'h140: csr_sscratch <= exu_csr_write_req_slv.pkt.val;
                    12'h141: csr_sepc <= exu_csr_write_req_slv.pkt.val;
                    12'h142: csr_scause <= exu_csr_write_req_slv.pkt.val;
                    12'h143: csr_stval <= exu_csr_write_req_slv.pkt.val;
                    12'h144: csr_mip <= (csr_mip & ~32'h00002222) | (exu_csr_write_req_slv.pkt.val & 32'h00002222);
                    12'h14d: csr_stimecmp <= exu_csr_write_req_slv.pkt.val;
                    12'h15d: csr_stimecmph <= exu_csr_write_req_slv.pkt.val;
                    12'h180: csr_satp <= exu_csr_write_req_slv.pkt.val;
                    12'h300: csr_mstatus <= exu_csr_write_req_slv.pkt.val;
                    12'h301: csr_misa <= exu_csr_write_req_slv.pkt.val;
                    12'h302: csr_medeleg <= exu_csr_write_req_slv.pkt.val;
                    12'h303: csr_mideleg <= exu_csr_write_req_slv.pkt.val;
                    12'h304: csr_mie <= exu_csr_write_req_slv.pkt.val;
                    12'h305: csr_mtvec <= exu_csr_write_req_slv.pkt.val;
                    12'h306: csr_mcounteren <= exu_csr_write_req_slv.pkt.val;
                    12'h30a: csr_menvcfg <= exu_csr_write_req_slv.pkt.val;
                    12'h310: csr_mstatush <= exu_csr_write_req_slv.pkt.val;
                    12'h31a: csr_menvcfgh <= exu_csr_write_req_slv.pkt.val;
                    12'h320: csr_mcountinhibit <= exu_csr_write_req_slv.pkt.val;
                    12'h321: csr_mcyclecfg <= exu_csr_write_req_slv.pkt.val;
                    12'h322: csr_minstretcfg <= exu_csr_write_req_slv.pkt.val;
                    12'h323: csr_mhpmevent3 <= exu_csr_write_req_slv.pkt.val;
                    12'h324: csr_mhpmevent4 <= exu_csr_write_req_slv.pkt.val;
                    12'h325: csr_mhpmevent5 <= exu_csr_write_req_slv.pkt.val;
                    12'h326: csr_mhpmevent6 <= exu_csr_write_req_slv.pkt.val;
                    12'h327: csr_mhpmevent7 <= exu_csr_write_req_slv.pkt.val;
                    12'h328: csr_mhpmevent8 <= exu_csr_write_req_slv.pkt.val;
                    12'h329: csr_mhpmevent9 <= exu_csr_write_req_slv.pkt.val;
                    12'h32a: csr_mhpmevent10 <= exu_csr_write_req_slv.pkt.val;
                    12'h32b: csr_mhpmevent11 <= exu_csr_write_req_slv.pkt.val;
                    12'h32c: csr_mhpmevent12 <= exu_csr_write_req_slv.pkt.val;
                    12'h32d: csr_mhpmevent13 <= exu_csr_write_req_slv.pkt.val;
                    12'h32e: csr_mhpmevent14 <= exu_csr_write_req_slv.pkt.val;
                    12'h32f: csr_mhpmevent15 <= exu_csr_write_req_slv.pkt.val;
                    12'h330: csr_mhpmevent16 <= exu_csr_write_req_slv.pkt.val;
                    12'h331: csr_mhpmevent17 <= exu_csr_write_req_slv.pkt.val;
                    12'h332: csr_mhpmevent18 <= exu_csr_write_req_slv.pkt.val;
                    12'h333: csr_mhpmevent19 <= exu_csr_write_req_slv.pkt.val;
                    12'h334: csr_mhpmevent20 <= exu_csr_write_req_slv.pkt.val;
                    12'h335: csr_mhpmevent21 <= exu_csr_write_req_slv.pkt.val;
                    12'h336: csr_mhpmevent22 <= exu_csr_write_req_slv.pkt.val;
                    12'h337: csr_mhpmevent23 <= exu_csr_write_req_slv.pkt.val;
                    12'h338: csr_mhpmevent24 <= exu_csr_write_req_slv.pkt.val;
                    12'h339: csr_mhpmevent25 <= exu_csr_write_req_slv.pkt.val;
                    12'h33a: csr_mhpmevent26 <= exu_csr_write_req_slv.pkt.val;
                    12'h33b: csr_mhpmevent27 <= exu_csr_write_req_slv.pkt.val;
                    12'h33c: csr_mhpmevent28 <= exu_csr_write_req_slv.pkt.val;
                    12'h33d: csr_mhpmevent29 <= exu_csr_write_req_slv.pkt.val;
                    12'h33e: csr_mhpmevent30 <= exu_csr_write_req_slv.pkt.val;
                    12'h33f: csr_mhpmevent31 <= exu_csr_write_req_slv.pkt.val;
                    12'h721: csr_mcyclecfgh <= exu_csr_write_req_slv.pkt.val;
                    12'h722: csr_minstretcfgh <= exu_csr_write_req_slv.pkt.val;
                    12'h723: csr_mhpmevent3h <= exu_csr_write_req_slv.pkt.val;
                    12'h724: csr_mhpmevent4h <= exu_csr_write_req_slv.pkt.val;
                    12'h725: csr_mhpmevent5h <= exu_csr_write_req_slv.pkt.val;
                    12'h726: csr_mhpmevent6h <= exu_csr_write_req_slv.pkt.val;
                    12'h727: csr_mhpmevent7h <= exu_csr_write_req_slv.pkt.val;
                    12'h728: csr_mhpmevent8h <= exu_csr_write_req_slv.pkt.val;
                    12'h729: csr_mhpmevent9h <= exu_csr_write_req_slv.pkt.val;
                    12'h72a: csr_mhpmevent10h <= exu_csr_write_req_slv.pkt.val;
                    12'h72b: csr_mhpmevent11h <= exu_csr_write_req_slv.pkt.val;
                    12'h72c: csr_mhpmevent12h <= exu_csr_write_req_slv.pkt.val;
                    12'h72d: csr_mhpmevent13h <= exu_csr_write_req_slv.pkt.val;
                    12'h72e: csr_mhpmevent14h <= exu_csr_write_req_slv.pkt.val;
                    12'h72f: csr_mhpmevent15h <= exu_csr_write_req_slv.pkt.val;
                    12'h730: csr_mhpmevent16h <= exu_csr_write_req_slv.pkt.val;
                    12'h731: csr_mhpmevent17h <= exu_csr_write_req_slv.pkt.val;
                    12'h732: csr_mhpmevent18h <= exu_csr_write_req_slv.pkt.val;
                    12'h733: csr_mhpmevent19h <= exu_csr_write_req_slv.pkt.val;
                    12'h734: csr_mhpmevent20h <= exu_csr_write_req_slv.pkt.val;
                    12'h735: csr_mhpmevent21h <= exu_csr_write_req_slv.pkt.val;
                    12'h736: csr_mhpmevent22h <= exu_csr_write_req_slv.pkt.val;
                    12'h737: csr_mhpmevent23h <= exu_csr_write_req_slv.pkt.val;
                    12'h738: csr_mhpmevent24h <= exu_csr_write_req_slv.pkt.val;
                    12'h739: csr_mhpmevent25h <= exu_csr_write_req_slv.pkt.val;
                    12'h73a: csr_mhpmevent26h <= exu_csr_write_req_slv.pkt.val;
                    12'h73b: csr_mhpmevent27h <= exu_csr_write_req_slv.pkt.val;
                    12'h73c: csr_mhpmevent28h <= exu_csr_write_req_slv.pkt.val;
                    12'h73d: csr_mhpmevent29h <= exu_csr_write_req_slv.pkt.val;
                    12'h73e: csr_mhpmevent30h <= exu_csr_write_req_slv.pkt.val;
                    12'h73f: csr_mhpmevent31h <= exu_csr_write_req_slv.pkt.val;
                    12'h340: csr_mscratch <= exu_csr_write_req_slv.pkt.val;
                    12'h341: csr_mepc <= exu_csr_write_req_slv.pkt.val;
                    12'h342: csr_mcause <= exu_csr_write_req_slv.pkt.val;
                    12'h343: csr_mtval <= exu_csr_write_req_slv.pkt.val;
                    12'h344: csr_mip <= exu_csr_write_req_slv.pkt.val;
                    12'h34a: csr_mtinst <= exu_csr_write_req_slv.pkt.val;
                    12'h34b: csr_mtval2 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a0: csr_pmpcfg0 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a1: csr_pmpcfg1 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a2: csr_pmpcfg2 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a3: csr_pmpcfg3 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a4: csr_pmpcfg4 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a5: csr_pmpcfg5 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a6: csr_pmpcfg6 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a7: csr_pmpcfg7 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a8: csr_pmpcfg8 <= exu_csr_write_req_slv.pkt.val;
                    12'h3a9: csr_pmpcfg9 <= exu_csr_write_req_slv.pkt.val;
                    12'h3aa: csr_pmpcfg10 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ab: csr_pmpcfg11 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ac: csr_pmpcfg12 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ad: csr_pmpcfg13 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ae: csr_pmpcfg14 <= exu_csr_write_req_slv.pkt.val;
                    12'h3af: csr_pmpcfg15 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b0: csr_pmpaddr0 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b1: csr_pmpaddr1 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b2: csr_pmpaddr2 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b3: csr_pmpaddr3 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b4: csr_pmpaddr4 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b5: csr_pmpaddr5 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b6: csr_pmpaddr6 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b7: csr_pmpaddr7 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b8: csr_pmpaddr8 <= exu_csr_write_req_slv.pkt.val;
                    12'h3b9: csr_pmpaddr9 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ba: csr_pmpaddr10 <= exu_csr_write_req_slv.pkt.val;
                    12'h3bb: csr_pmpaddr11 <= exu_csr_write_req_slv.pkt.val;
                    12'h3bc: csr_pmpaddr12 <= exu_csr_write_req_slv.pkt.val;
                    12'h3bd: csr_pmpaddr13 <= exu_csr_write_req_slv.pkt.val;
                    12'h3be: csr_pmpaddr14 <= exu_csr_write_req_slv.pkt.val;
                    12'h3bf: csr_pmpaddr15 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c0: csr_pmpaddr16 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c1: csr_pmpaddr17 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c2: csr_pmpaddr18 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c3: csr_pmpaddr19 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c4: csr_pmpaddr20 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c5: csr_pmpaddr21 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c6: csr_pmpaddr22 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c7: csr_pmpaddr23 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c8: csr_pmpaddr24 <= exu_csr_write_req_slv.pkt.val;
                    12'h3c9: csr_pmpaddr25 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ca: csr_pmpaddr26 <= exu_csr_write_req_slv.pkt.val;
                    12'h3cb: csr_pmpaddr27 <= exu_csr_write_req_slv.pkt.val;
                    12'h3cc: csr_pmpaddr28 <= exu_csr_write_req_slv.pkt.val;
                    12'h3cd: csr_pmpaddr29 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ce: csr_pmpaddr30 <= exu_csr_write_req_slv.pkt.val;
                    12'h3cf: csr_pmpaddr31 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d0: csr_pmpaddr32 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d1: csr_pmpaddr33 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d2: csr_pmpaddr34 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d3: csr_pmpaddr35 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d4: csr_pmpaddr36 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d5: csr_pmpaddr37 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d6: csr_pmpaddr38 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d7: csr_pmpaddr39 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d8: csr_pmpaddr40 <= exu_csr_write_req_slv.pkt.val;
                    12'h3d9: csr_pmpaddr41 <= exu_csr_write_req_slv.pkt.val;
                    12'h3da: csr_pmpaddr42 <= exu_csr_write_req_slv.pkt.val;
                    12'h3db: csr_pmpaddr43 <= exu_csr_write_req_slv.pkt.val;
                    12'h3dc: csr_pmpaddr44 <= exu_csr_write_req_slv.pkt.val;
                    12'h3dd: csr_pmpaddr45 <= exu_csr_write_req_slv.pkt.val;
                    12'h3de: csr_pmpaddr46 <= exu_csr_write_req_slv.pkt.val;
                    12'h3df: csr_pmpaddr47 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e0: csr_pmpaddr48 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e1: csr_pmpaddr49 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e2: csr_pmpaddr50 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e3: csr_pmpaddr51 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e4: csr_pmpaddr52 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e5: csr_pmpaddr53 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e6: csr_pmpaddr54 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e7: csr_pmpaddr55 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e8: csr_pmpaddr56 <= exu_csr_write_req_slv.pkt.val;
                    12'h3e9: csr_pmpaddr57 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ea: csr_pmpaddr58 <= exu_csr_write_req_slv.pkt.val;
                    12'h3eb: csr_pmpaddr59 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ec: csr_pmpaddr60 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ed: csr_pmpaddr61 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ee: csr_pmpaddr62 <= exu_csr_write_req_slv.pkt.val;
                    12'h3ef: csr_pmpaddr63 <= exu_csr_write_req_slv.pkt.val;
                    12'h7a0: csr_tselect <= exu_csr_write_req_slv.pkt.val;
                    12'h7a4: csr_tinfo <= exu_csr_write_req_slv.pkt.val;
                    12'hb03: csr_mhpmcounter3 <= exu_csr_write_req_slv.pkt.val;
                    12'hb04: csr_mhpmcounter4 <= exu_csr_write_req_slv.pkt.val;
                    12'hb05: csr_mhpmcounter5 <= exu_csr_write_req_slv.pkt.val;
                    12'hb06: csr_mhpmcounter6 <= exu_csr_write_req_slv.pkt.val;
                    12'hb07: csr_mhpmcounter7 <= exu_csr_write_req_slv.pkt.val;
                    12'hb08: csr_mhpmcounter8 <= exu_csr_write_req_slv.pkt.val;
                    12'hb09: csr_mhpmcounter9 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0a: csr_mhpmcounter10 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0b: csr_mhpmcounter11 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0c: csr_mhpmcounter12 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0d: csr_mhpmcounter13 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0e: csr_mhpmcounter14 <= exu_csr_write_req_slv.pkt.val;
                    12'hb0f: csr_mhpmcounter15 <= exu_csr_write_req_slv.pkt.val;
                    12'hb10: csr_mhpmcounter16 <= exu_csr_write_req_slv.pkt.val;
                    12'hb11: csr_mhpmcounter17 <= exu_csr_write_req_slv.pkt.val;
                    12'hb12: csr_mhpmcounter18 <= exu_csr_write_req_slv.pkt.val;
                    12'hb13: csr_mhpmcounter19 <= exu_csr_write_req_slv.pkt.val;
                    12'hb14: csr_mhpmcounter20 <= exu_csr_write_req_slv.pkt.val;
                    12'hb15: csr_mhpmcounter21 <= exu_csr_write_req_slv.pkt.val;
                    12'hb16: csr_mhpmcounter22 <= exu_csr_write_req_slv.pkt.val;
                    12'hb17: csr_mhpmcounter23 <= exu_csr_write_req_slv.pkt.val;
                    12'hb18: csr_mhpmcounter24 <= exu_csr_write_req_slv.pkt.val;
                    12'hb19: csr_mhpmcounter25 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1a: csr_mhpmcounter26 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1b: csr_mhpmcounter27 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1c: csr_mhpmcounter28 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1d: csr_mhpmcounter29 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1e: csr_mhpmcounter30 <= exu_csr_write_req_slv.pkt.val;
                    12'hb1f: csr_mhpmcounter31 <= exu_csr_write_req_slv.pkt.val;
                    12'hb83: csr_mhpmcounter3h <= exu_csr_write_req_slv.pkt.val;
                    12'hb84: csr_mhpmcounter4h <= exu_csr_write_req_slv.pkt.val;
                    12'hb85: csr_mhpmcounter5h <= exu_csr_write_req_slv.pkt.val;
                    12'hb86: csr_mhpmcounter6h <= exu_csr_write_req_slv.pkt.val;
                    12'hb87: csr_mhpmcounter7h <= exu_csr_write_req_slv.pkt.val;
                    12'hb88: csr_mhpmcounter8h <= exu_csr_write_req_slv.pkt.val;
                    12'hb89: csr_mhpmcounter9h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8a: csr_mhpmcounter10h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8b: csr_mhpmcounter11h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8c: csr_mhpmcounter12h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8d: csr_mhpmcounter13h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8e: csr_mhpmcounter14h <= exu_csr_write_req_slv.pkt.val;
                    12'hb8f: csr_mhpmcounter15h <= exu_csr_write_req_slv.pkt.val;
                    12'hb90: csr_mhpmcounter16h <= exu_csr_write_req_slv.pkt.val;
                    12'hb91: csr_mhpmcounter17h <= exu_csr_write_req_slv.pkt.val;
                    12'hb92: csr_mhpmcounter18h <= exu_csr_write_req_slv.pkt.val;
                    12'hb93: csr_mhpmcounter19h <= exu_csr_write_req_slv.pkt.val;
                    12'hb94: csr_mhpmcounter20h <= exu_csr_write_req_slv.pkt.val;
                    12'hb95: csr_mhpmcounter21h <= exu_csr_write_req_slv.pkt.val;
                    12'hb96: csr_mhpmcounter22h <= exu_csr_write_req_slv.pkt.val;
                    12'hb97: csr_mhpmcounter23h <= exu_csr_write_req_slv.pkt.val;
                    12'hb98: csr_mhpmcounter24h <= exu_csr_write_req_slv.pkt.val;
                    12'hb99: csr_mhpmcounter25h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9a: csr_mhpmcounter26h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9b: csr_mhpmcounter27h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9c: csr_mhpmcounter28h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9d: csr_mhpmcounter29h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9e: csr_mhpmcounter30h <= exu_csr_write_req_slv.pkt.val;
                    12'hb9f: csr_mhpmcounter31h <= exu_csr_write_req_slv.pkt.val;
                    default: ;
                endcase
            end
        end
    end

    always_comb begin
        csr_exu_read_rsp_mst.pkt.val = '0;
        csr_exu_read_rsp_mst.pkt.ok = 1'b0;
        case (exu_csr_read_req_slv.pkt.addr)
            12'h100: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = (csr_mstatus & 32'h800de762);
            end
            12'h104: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = (csr_mie & 32'h00002222);
            end
            12'h105: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_stvec;
            end
            12'h106: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_scounteren;
            end
            12'h10a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_senvcfg;
            end
            12'h140: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_sscratch;
            end
            12'h141: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_sepc;
            end
            12'h142: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_scause;
            end
            12'h143: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_stval;
            end
            12'h144: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = (csr_mip & 32'h00002222);
            end
            12'h14d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_stimecmp;
            end
            12'h15d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_stimecmph;
            end
            12'h180: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_satp;
            end
            12'h300: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mstatus;
            end
            12'h301: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_misa;
            end
            12'h302: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_medeleg;
            end
            12'h303: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mideleg;
            end
            12'h304: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mie;
            end
            12'h305: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mtvec;
            end
            12'h306: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mcounteren;
            end
            12'h30a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_menvcfg;
            end
            12'h310: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mstatush;
            end
            12'h31a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_menvcfgh;
            end
            12'h320: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mcountinhibit;
            end
            12'h321: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mcyclecfg;
            end
            12'h322: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_minstretcfg;
            end
            12'h323: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent3;
            end
            12'h324: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent4;
            end
            12'h325: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent5;
            end
            12'h326: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent6;
            end
            12'h327: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent7;
            end
            12'h328: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent8;
            end
            12'h329: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent9;
            end
            12'h32a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent10;
            end
            12'h32b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent11;
            end
            12'h32c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent12;
            end
            12'h32d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent13;
            end
            12'h32e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent14;
            end
            12'h32f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent15;
            end
            12'h330: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent16;
            end
            12'h331: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent17;
            end
            12'h332: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent18;
            end
            12'h333: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent19;
            end
            12'h334: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent20;
            end
            12'h335: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent21;
            end
            12'h336: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent22;
            end
            12'h337: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent23;
            end
            12'h338: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent24;
            end
            12'h339: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent25;
            end
            12'h33a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent26;
            end
            12'h33b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent27;
            end
            12'h33c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent28;
            end
            12'h33d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent29;
            end
            12'h33e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent30;
            end
            12'h33f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent31;
            end
            12'h721: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mcyclecfgh;
            end
            12'h722: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_minstretcfgh;
            end
            12'h723: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent3h;
            end
            12'h724: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent4h;
            end
            12'h725: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent5h;
            end
            12'h726: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent6h;
            end
            12'h727: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent7h;
            end
            12'h728: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent8h;
            end
            12'h729: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent9h;
            end
            12'h72a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent10h;
            end
            12'h72b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent11h;
            end
            12'h72c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent12h;
            end
            12'h72d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent13h;
            end
            12'h72e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent14h;
            end
            12'h72f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent15h;
            end
            12'h730: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent16h;
            end
            12'h731: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent17h;
            end
            12'h732: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent18h;
            end
            12'h733: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent19h;
            end
            12'h734: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent20h;
            end
            12'h735: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent21h;
            end
            12'h736: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent22h;
            end
            12'h737: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent23h;
            end
            12'h738: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent24h;
            end
            12'h739: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent25h;
            end
            12'h73a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent26h;
            end
            12'h73b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent27h;
            end
            12'h73c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent28h;
            end
            12'h73d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent29h;
            end
            12'h73e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent30h;
            end
            12'h73f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmevent31h;
            end
            12'h340: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mscratch;
            end
            12'h341: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mepc;
            end
            12'h342: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mcause;
            end
            12'h343: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mtval;
            end
            12'h344: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mip;
            end
            12'h34a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mtinst;
            end
            12'h34b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mtval2;
            end
            12'h3a0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg0;
            end
            12'h3a1: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg1;
            end
            12'h3a2: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg2;
            end
            12'h3a3: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg3;
            end
            12'h3a4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg4;
            end
            12'h3a5: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg5;
            end
            12'h3a6: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg6;
            end
            12'h3a7: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg7;
            end
            12'h3a8: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg8;
            end
            12'h3a9: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg9;
            end
            12'h3aa: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg10;
            end
            12'h3ab: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg11;
            end
            12'h3ac: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg12;
            end
            12'h3ad: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg13;
            end
            12'h3ae: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg14;
            end
            12'h3af: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpcfg15;
            end
            12'h3b0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr0;
            end
            12'h3b1: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr1;
            end
            12'h3b2: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr2;
            end
            12'h3b3: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr3;
            end
            12'h3b4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr4;
            end
            12'h3b5: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr5;
            end
            12'h3b6: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr6;
            end
            12'h3b7: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr7;
            end
            12'h3b8: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr8;
            end
            12'h3b9: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr9;
            end
            12'h3ba: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr10;
            end
            12'h3bb: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr11;
            end
            12'h3bc: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr12;
            end
            12'h3bd: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr13;
            end
            12'h3be: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr14;
            end
            12'h3bf: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr15;
            end
            12'h3c0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr16;
            end
            12'h3c1: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr17;
            end
            12'h3c2: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr18;
            end
            12'h3c3: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr19;
            end
            12'h3c4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr20;
            end
            12'h3c5: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr21;
            end
            12'h3c6: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr22;
            end
            12'h3c7: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr23;
            end
            12'h3c8: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr24;
            end
            12'h3c9: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr25;
            end
            12'h3ca: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr26;
            end
            12'h3cb: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr27;
            end
            12'h3cc: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr28;
            end
            12'h3cd: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr29;
            end
            12'h3ce: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr30;
            end
            12'h3cf: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr31;
            end
            12'h3d0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr32;
            end
            12'h3d1: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr33;
            end
            12'h3d2: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr34;
            end
            12'h3d3: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr35;
            end
            12'h3d4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr36;
            end
            12'h3d5: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr37;
            end
            12'h3d6: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr38;
            end
            12'h3d7: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr39;
            end
            12'h3d8: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr40;
            end
            12'h3d9: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr41;
            end
            12'h3da: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr42;
            end
            12'h3db: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr43;
            end
            12'h3dc: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr44;
            end
            12'h3dd: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr45;
            end
            12'h3de: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr46;
            end
            12'h3df: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr47;
            end
            12'h3e0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr48;
            end
            12'h3e1: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr49;
            end
            12'h3e2: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr50;
            end
            12'h3e3: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr51;
            end
            12'h3e4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr52;
            end
            12'h3e5: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr53;
            end
            12'h3e6: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr54;
            end
            12'h3e7: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr55;
            end
            12'h3e8: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr56;
            end
            12'h3e9: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr57;
            end
            12'h3ea: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr58;
            end
            12'h3eb: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr59;
            end
            12'h3ec: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr60;
            end
            12'h3ed: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr61;
            end
            12'h3ee: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr62;
            end
            12'h3ef: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_pmpaddr63;
            end
            12'h7a0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_tselect;
            end
            12'h7a4: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_tinfo;
            end
            12'hb03: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter3;
            end
            12'hb04: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter4;
            end
            12'hb05: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter5;
            end
            12'hb06: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter6;
            end
            12'hb07: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter7;
            end
            12'hb08: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter8;
            end
            12'hb09: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter9;
            end
            12'hb0a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter10;
            end
            12'hb0b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter11;
            end
            12'hb0c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter12;
            end
            12'hb0d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter13;
            end
            12'hb0e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter14;
            end
            12'hb0f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter15;
            end
            12'hb10: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter16;
            end
            12'hb11: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter17;
            end
            12'hb12: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter18;
            end
            12'hb13: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter19;
            end
            12'hb14: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter20;
            end
            12'hb15: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter21;
            end
            12'hb16: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter22;
            end
            12'hb17: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter23;
            end
            12'hb18: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter24;
            end
            12'hb19: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter25;
            end
            12'hb1a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter26;
            end
            12'hb1b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter27;
            end
            12'hb1c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter28;
            end
            12'hb1d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter29;
            end
            12'hb1e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter30;
            end
            12'hb1f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter31;
            end
            12'hb83: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter3h;
            end
            12'hb84: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter4h;
            end
            12'hb85: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter5h;
            end
            12'hb86: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter6h;
            end
            12'hb87: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter7h;
            end
            12'hb88: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter8h;
            end
            12'hb89: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter9h;
            end
            12'hb8a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter10h;
            end
            12'hb8b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter11h;
            end
            12'hb8c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter12h;
            end
            12'hb8d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter13h;
            end
            12'hb8e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter14h;
            end
            12'hb8f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter15h;
            end
            12'hb90: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter16h;
            end
            12'hb91: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter17h;
            end
            12'hb92: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter18h;
            end
            12'hb93: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter19h;
            end
            12'hb94: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter20h;
            end
            12'hb95: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter21h;
            end
            12'hb96: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter22h;
            end
            12'hb97: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter23h;
            end
            12'hb98: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter24h;
            end
            12'hb99: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter25h;
            end
            12'hb9a: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter26h;
            end
            12'hb9b: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter27h;
            end
            12'hb9c: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter28h;
            end
            12'hb9d: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter29h;
            end
            12'hb9e: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter30h;
            end
            12'hb9f: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhpmcounter31h;
            end
            12'hc00: begin
                csr_exu_read_rsp_mst.pkt.ok = 1'b1;
                csr_exu_read_rsp_mst.pkt.val = csr_cycle;
            end
            12'hc01: begin
                csr_exu_read_rsp_mst.pkt.ok = 1'b1;
                csr_exu_read_rsp_mst.pkt.val = csr_time;
            end
            12'hc02: begin
                csr_exu_read_rsp_mst.pkt.ok = 1'b1;
                csr_exu_read_rsp_mst.pkt.val = csr_instret;
            end
            12'hc80: begin
                csr_exu_read_rsp_mst.pkt.ok = 1'b1;
                csr_exu_read_rsp_mst.pkt.val = csr_cycleh;
            end
            12'hc81: begin
                csr_exu_read_rsp_mst.pkt.ok = 1'b1;
                csr_exu_read_rsp_mst.pkt.val = csr_timeh;
            end
            12'hda0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd1;
                csr_exu_read_rsp_mst.pkt.val = csr_scountovf;
            end
            12'hf11: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mvendorid;
            end
            12'hf12: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_marchid;
            end
            12'hf13: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mimpid;
            end
            12'hf14: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mhartid;
            end
            12'hfb0: begin
                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd3;
                csr_exu_read_rsp_mst.pkt.val = csr_mtopi;
            end
            default: ;
        endcase
    end

    always_comb begin
        csr_exu_write_rsp_mst.vld = exu_csr_write_req_slv.vld;
        csr_exu_write_rsp_mst.pkt.ok = 1'b0;
        case (exu_csr_write_req_slv.pkt.addr)
            12'h100: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h104: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h105: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h106: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h10a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h140: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h141: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h142: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h143: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h144: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h14d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h15d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h180: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd1;
            12'h300: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h301: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h302: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h303: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h304: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h305: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h306: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h30a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h310: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h31a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h320: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h321: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h322: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h323: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h324: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h325: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h326: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h327: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h328: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h329: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h32f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h330: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h331: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h332: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h333: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h334: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h335: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h336: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h337: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h338: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h339: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h33f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h721: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h722: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h723: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h724: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h725: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h726: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h727: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h728: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h729: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h72f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h730: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h731: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h732: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h733: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h734: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h735: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h736: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h737: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h738: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h739: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h73f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h340: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h341: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h342: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h343: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h344: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h34a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h34b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a1: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a2: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a3: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a5: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a6: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a7: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a8: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3a9: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3aa: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ab: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ac: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ad: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ae: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3af: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b1: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b2: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b3: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b5: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b6: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b7: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b8: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3b9: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ba: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3bb: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3bc: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3bd: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3be: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3bf: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c1: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c2: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c3: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c5: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c6: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c7: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c8: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3c9: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ca: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3cb: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3cc: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3cd: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ce: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3cf: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d1: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d2: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d3: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d5: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d6: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d7: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d8: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3d9: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3da: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3db: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3dc: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3dd: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3de: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3df: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e1: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e2: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e3: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e5: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e6: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e7: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e8: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3e9: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ea: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3eb: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ec: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ed: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ee: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h3ef: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h7a0: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'h7a4: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb03: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb04: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb05: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb06: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb07: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb08: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb09: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb0f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb10: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb11: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb12: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb13: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb14: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb15: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb16: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb17: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb18: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb19: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb1f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb83: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb84: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb85: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb86: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb87: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb88: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb89: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb8f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb90: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb91: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb92: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb93: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb94: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb95: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb96: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb97: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb98: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb99: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9a: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9b: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9c: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9d: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9e: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            12'hb9f: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd3;
            default: ;
        endcase
    end

    assign csr_trap_write_rsp_mst.vld = trap_csr_write_req_slv.vld;
    assign csr_trap_write_rsp_mst.pkt.ok = 1'b1;
    assign csr_trap_state_mst.pkt.mstatus = csr_mstatus;
    assign csr_trap_state_mst.pkt.mip = csr_mip;
    assign csr_trap_state_mst.pkt.mie = csr_mie;
    assign csr_trap_state_mst.pkt.mtvec = csr_mtvec;
    assign csr_trap_state_mst.pkt.mepc = csr_mepc;
    assign csr_trap_state_mst.pkt.medeleg = csr_medeleg;
    assign csr_trap_state_mst.pkt.mideleg = csr_mideleg;
    assign csr_trap_state_mst.pkt.sstatus = (csr_mstatus & 32'h800de762);
    assign csr_trap_state_mst.pkt.sip = (csr_mip & 32'h00002222);
    assign csr_trap_state_mst.pkt.sie = (csr_mie & 32'h00002222);
    assign csr_trap_state_mst.pkt.stvec = csr_stvec;
    assign csr_trap_state_mst.pkt.sepc = csr_sepc;
    assign csr_mmu_state_mst.pkt.satp = csr_satp;
    assign csr_mmu_state_mst.pkt.mstatus = csr_mstatus;
    assign csr_lsu_state_mst.pkt.satp = csr_satp;
endmodule
