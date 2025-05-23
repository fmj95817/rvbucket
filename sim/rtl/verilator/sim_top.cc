#include "Vsim_top.h"

using namespace sc_core;

#define NS(t) sc_time(t, SC_NS)

static bool rst_n_val(const sc_time &ts)
{
    if (ts >= NS(3) && ts < NS(10)) {
        return false;
    } else {
        return true;
    }
}

int sc_main(int argc, char *argv[])
{
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);
    Verilated::randReset(2);

    sc_clock clk{"clk", 10, SC_NS, 0.5, 3, SC_NS, true};
    sc_signal<bool> rst_n;

    Vsim_top sim_top{"top"};
    sim_top.clk(clk);
    sim_top.rst_n(rst_n);

    while (!Verilated::gotFinish()) {
        auto ts = sc_time_stamp();
        rst_n = rst_n_val(ts);
        sc_start(1, SC_NS);
    }

    return 0;
  }