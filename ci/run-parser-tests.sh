#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."

./build.sh
for corpus in "$@"; do
  src/FaultGenerator/test/testParser.sh "$corpus" build/src/FaultGenerator/test/LibertyParserBin
done
