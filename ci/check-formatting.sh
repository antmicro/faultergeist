#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$0")/.."

find ./src -regextype posix-egrep -regex '.*(\.cpp|\.h)' ! -name 'sv_vpi_user.h' -print0 |
  xargs -0 clang-format --dry-run --Werror
