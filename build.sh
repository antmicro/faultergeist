#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cmake -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVEER_JSON_NETLIST_CACHE_PATH="" \
  -DVEER_VERILOG_NETLIST_CACHE_PATH="" \
  -DFI_E2E_FORCE_SETUP_VEER=OFF \
  -DCMAKE_CXX_FLAGS='-Wall -Wextra'

cmake --build build -j "$(nproc)"
