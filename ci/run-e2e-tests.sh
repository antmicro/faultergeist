#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

suite="${1:?usage: $0 <suite>}"

cd "$(dirname "$0")/.."
export PATH="$PWD/third_party/yosys/build:$PATH"

cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DFI_E2E_GENERATOR_EXTRA_DEBUG=3 \
  -DFI_E2E_FORCE_SETUP_VEER="${FI_E2E_FORCE_SETUP_VEER:-OFF}" \
  -DVEER_JSON_NETLIST_CACHE_PATH="${VEER_JSON_NETLIST_CACHE_PATH:-}" \
  -DVEER_VERILOG_NETLIST_CACHE_PATH="${VEER_VERILOG_NETLIST_CACHE_PATH:-}"
cmake --build build -j "$(nproc)" 2>&1 | tee build/cmake-build.log
ctest --test-dir build --output-on-failure --tests-regex "e2e.$suite"
