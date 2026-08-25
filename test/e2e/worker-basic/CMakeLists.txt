# SPDX-License-Identifier: Apache-2.0

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
define_worker_testbench_vars()

set(JSON_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.json")
set(VERILOG_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.v")

# First produce the Yosys JSON netlist consumed by faultergeist-gen.
fi_add_yosys_json(fi_e2e_worker_basic_netlist)

add_subdirectory(baseline)
add_subdirectory(random)
add_subdirectory(bendel)
add_subdirectory(weibull)
