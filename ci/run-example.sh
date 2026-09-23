#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."
export PATH="$PWD/third_party/yosys/build:$PATH"

example/run.sh
