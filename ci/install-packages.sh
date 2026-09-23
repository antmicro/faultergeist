#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get -yq update >/dev/null
apt-get -yq upgrade
apt-get -yq install \
  7zip autoconf bison build-essential ccache gcc clang-format cmake curl file flex gawk git \
  graphviz grep gzip help2man libffi-dev libfl-dev libreadline-dev lld make pkg-config python3 \
  tcl-dev xdot zlib1g-dev zstd
