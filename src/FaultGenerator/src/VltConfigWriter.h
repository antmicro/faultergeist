// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <ostream>
#include <span>

struct Signal;

class VltConfigWriter {
    std::filesystem::path path;
    std::ostream* stream = nullptr;
    std::span<const Signal> signals;
    bool all_forceable = false;
    bool all_public_flat_rw = false;

   public:
    VltConfigWriter(
        std::filesystem::path path,
        std::span<const Signal> signals,
        bool all_forceable = false,
        bool all_public_flat_rw = false
    )
        : path(std::move(path)),
          signals(signals),
          all_forceable(all_forceable),
          all_public_flat_rw(all_public_flat_rw) {}
    VltConfigWriter(
        std::ostream& stream,
        std::span<const Signal> signals,
        bool all_forceable = false,
        bool all_public_flat_rw = false
    )
        : stream(&stream),
          signals(signals),
          all_forceable(all_forceable),
          all_public_flat_rw(all_public_flat_rw) {}

    void write();
    static void write(
        std::ostream&,
        std::span<const Signal>,
        bool all_forceable = false,
        bool all_public_flat_rw = false
    );
};
