#!/bin/bash

set -euox pipefail

VERILATOR=""
if [[ -z "$VERILATOR_ROOT" ]]; then
  VERILATOR="$(which verilator)"
else
  VERILATOR="$VERILATOR_ROOT/bin/verilator"
fi

cat <<EOF
# ==============================================================================
# The following instructions provide a demonstration of the faultergeist plugin
# using the worker core as an example.
# ==============================================================================
EOF

# It should be run in the directory it is in
cd "$(dirname "$0")"


cat <<EOF
# ==============================================================================
# Before the tool can be ran, the project must be configured and built.
# ==============================================================================
EOF
cmake -B build -S .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DBUILD_TESTING=OFF
cmake --build build -j "$(nproc)"


cat <<EOF
# ==============================================================================
# The next step is to generate a list of signals to inject. We can do this
# using Yosys.
# ==============================================================================
EOF
yosys <<EOF
  read_verilog worker/dff_worker.v
  proc
  rename -wire
  write_json netlist.json
  exit
EOF


cat <<EOF
# ==============================================================================
# Once we have the netlist, we can proceed to execute the faultergeist-gen to
# generate a fault campaign. To do so, we pass a config file, like the one
# prepared in this directory. At this point, we select the desired model, as
# well as other parameters for fault generation.
# ==============================================================================
EOF
build/src/FaultGenerator/faultergeist-gen --config_file=config.json.in


cat <<EOF
# ==============================================================================
# The next step is to prepare the simulation code. In order to inject the
# faults during the simulation, Verilator requires a few extra arguments.
# Here is the explanation of what they do:
# - '--binary', '-j $(nproc)' - normal flags typically passed to Verilator.
#   They cause Verilator to generate all simulation files and build them into a
#   single binary.
# - '--vpi' '--public-flat-rw' - flags enabling the Verilator features that the
#   fault injection relies on
# - worker/... - SystemVerilog files which should be replaced with files from
#   your design
# - faultergeist-inject.sv - fault injection plugin adapter written in
#   SystemVerilog
# - '-LDFLAGS "-Lbuild/src/FaultInjector"' '-lfaultergeist-inject'
#   - precompiled part of the fault injection plugin
# - '-DFAULT_INJECTION_ENABLE' - flag enabling the fault injection in the
#   simulation
# - -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_campaign_out.csv\"" - flag
#   pointing Verilator towards the file containing the campaign
# - -DVCD_OUTPUT_PATH - path to VCD file where waveforms from the simulation
#   will be dumped
# ==============================================================================
EOF
$VERILATOR \
    --binary -j "$(nproc)" \
    --vpi --public-flat-rw \
    worker/config.vlt worker/top.v worker/worker.v worker/comb_worker.v worker/dff_worker.v \
    ../src/FaultInjector/faultergeist-inject.sv \
    -LDFLAGS "-L$(pwd)/build/src/FaultInjector -lfaultergeist-inject" \
    -DFAULT_INJECTION_ENABLE \
    -DFAULT_INJECTION_CAMPAIGN_FILE="\"fault_campaign_out.csv\"" \
    -DVCD_OUTPUT_PATH="\"vlt_dump.vcd\""


cat <<EOF
# ==============================================================================
# The last step left is to run the simulation and watch how faults impact the
# design.
# ==============================================================================
EOF
obj_dir/Vtop
