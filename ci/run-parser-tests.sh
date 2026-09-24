#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."

./build.sh
for corpus in "$@"; do
  if [[ ! -d "$corpus" ]]; then
    echo "'$corpus' does not exist"
    exit 1
  fi
  src/FaultGenerator/test/testParser.sh "$corpus" build/src/FaultGenerator/test/LibertyParserBin
done
