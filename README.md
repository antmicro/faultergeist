# Faultergeist
Copyright (c) 2026 Antmicro

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/logo-readme-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="assets/logo-readme-light.png">
    <img src="assets/logo-readme-light.png">
  </picture>
</p>

A fault injection framework for hardware design simulation, including a plugin
for VPI-compatible simulators. Generates fault campaigns based on a netlist and
injects them into a simulation via VPI.

## Setup
To build the project, run:

```sh
./build.sh
```

The workflow below uses an installed copy. Build it and choose an installation
prefix:

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix "$PWD/install"
```
To use the project without installing it, follow the source-tree workflow in [`example/run.sh`](example/run.sh).

To build and run unit tests:

```sh
./build.sh -DBUILD_TESTING=ON
ctest --test-dir build
```

## Usage
To generate and simulate a fault campaign:

1. Run synthesis to get a JSON netlist file with mapped DFFs.

```
yosys <<EOF
	read_verilog [... SV sources ...]
	proc
	rename -wire
	write_json netlist.json
	exit
EOF
```

2. Emit faults.

```bash
faultergeist-gen \
  --sig_path_prefix="top" \
  --top_module="instance" \
  --top_instance="instance_name" \
  --netlist_path="netlist.json" \
  --fault_campaign_out="fault_campaign_out.csv"
```

3. Initialize FI module in test bench code.
```verilog
module top;
...
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi("fault_campaign_out.csv");
`endif
...
endmodule
```

4. Invoke Verilator with the installed FaultInjection source and library.

```bash
verilator \
    [... verilator flags ...] \
    --vpi --public-flat-rw \ # Enable VPI and expose signals for injection
    [... SV sources ...] \
    $(pkg-config --libs faultergeist) \ # Include and link FI library
    -DFAULT_INJECTION_ENABLE # Enable FI
```

The installed `faultergeist.pc` lets `pkg-config` locate the static library and
SystemVerilog source. System installation prefixes are discovered automatically;
set `PKG_CONFIG_PATH` as above for a custom prefix.
