// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: main() simulation loop, created with --main

#include <filesystem>
#include <memory>
#include <string>
#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "verilated_cov.h"
#include "verilated_vpi.h"

#ifndef VCD_OUTPUT_PATH
#error "VCD_OUTPUT_PATH must be defined during Verilator compilation"
#endif

// User VPI code adds to this array to get startup main() callbacks
extern "C" void (*vlog_startup_routines[])() VL_ATTR_WEAK;

//======================

int main(int argc, char** argv, char**) {
    // Setup context, defaults, and parse command line
    Verilated::debug(0);
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->traceEverOn(true);
    contextp->threads(1);
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating
    const std::unique_ptr<Vtop> topp{new Vtop{contextp.get(), ""}};
    topp->clk = 0;

    // Hook VPI startup routines and invoke callback
    if (vlog_startup_routines) {
        for (auto routinep = &vlog_startup_routines[0]; *routinep; routinep++) {
            (*routinep)();
        }
    }
    VerilatedVpi::callCbs(cbStartOfSimulation);

    auto tfp = std::make_unique<VerilatedVcdC>();
    topp->trace(tfp.get(), 99);  // Trace 99 levels of hierarchy
    const std::filesystem::path vcdPath{VCD_OUTPUT_PATH};
    if (vcdPath.has_parent_path()) {
        std::filesystem::create_directories(vcdPath.parent_path());
    }
    const std::string vcdPathString = vcdPath.string();
    tfp->open(vcdPathString.c_str());

    // Simulate until $finish
    while (VL_LIKELY(!contextp->gotFinish())) {
        topp->clk = !topp->clk;
        // VPI callbacks
        VerilatedVpi::callTimedCbs();
        VerilatedVpi::callCbs(cbNextSimTime);
        VerilatedVpi::callCbs(cbAtStartOfSimTime);
        // Evaluate model
        topp->eval();
        if (tfp) {
            tfp->dump(contextp->time());
        }
        // VPI callbacks
        VerilatedVpi::callValueCbs();
        VerilatedVpi::callCbs(cbAtEndOfSimTime);
        VerilatedVpi::callCbs(cbReadWriteSynch);
        VerilatedVpi::callCbs(cbReadOnlySynch);
        // Advance time
        const uint64_t nextVpiDeadline = VerilatedVpi::cbNextDeadline();
        if (!topp->eventsPending() && nextVpiDeadline == ~0ULL) {
            break;
        }
        if (topp->eventsPending()) {
            contextp->time(std::min(topp->nextTimeSlot(), nextVpiDeadline));
        } else {
            contextp->time(nextVpiDeadline);
        }
    }

    if (VL_LIKELY(!contextp->gotFinish())) {
        VL_DEBUG_IF(VL_PRINTF("+ Exiting without $finish; no events left\n"););
    }

    // Execute 'final' processes
    topp->final();
    VerilatedVpi::callCbs(cbEndOfSimulation);

    contextp->coveragep()->write();

    // Print statistical summary report
    contextp->statsPrintSummary();

    return 0;
}

