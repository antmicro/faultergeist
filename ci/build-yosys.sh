#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."

git submodule update --init --recursive third_party/yosys third_party/yosys-slang

cmake -B third_party/yosys/build third_party/yosys \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build third_party/yosys/build --parallel "$(nproc)"

make -C third_party/yosys-slang -j "$(nproc)" \
  CXX="ccache ${CXX:-g++}" \
  YOSYS_PREFIX="$PWD/third_party/yosys/build/"
mkdir -p third_party/yosys/build/share/plugins
cp third_party/yosys-slang/build/slang.so third_party/yosys/build/share/plugins
