#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

# Latest as of 21.09.2026
VERILATOR_REV=46774fded9a960ca5f5110d6ff099e73fe08f86c
VERILATOR_REPO="${VERILATOR_REPO:-https://github.com/verilator/verilator.git}"
VERILATOR_ROOT="${VERILATOR_ROOT:-$PWD/verilator}"

git clone "$VERILATOR_REPO" "$VERILATOR_ROOT"
cd "$VERILATOR_ROOT"
git checkout "$VERILATOR_REV"
git show -s
autoconf
./configure --prefix="$VERILATOR_ROOT"
make -j "$(nproc)" CC="ccache ${CC:-gcc}" CXX="ccache ${CXX:-g++}"
