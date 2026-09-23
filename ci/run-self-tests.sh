#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."
export PATH="$PWD/third_party/yosys/build:$PATH"

cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DENABLE_WEXTRA=ON \
  -DFI_E2E_GENERATOR_EXTRA_DEBUG=3
cmake --build build -j "$(nproc)" 2>&1 | tee build/cmake-build.log
ctest --test-dir build --output-on-failure --no-tests=error --tests-regex 'unit|integration'
