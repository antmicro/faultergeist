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

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

class Wire {
   public:
    std::string name;
    std::string hdlname;
    std::uint32_t width;
    std::uint32_t offset = 0;
    bool is_port = false;

    friend std::ostream& operator<<(std::ostream& os, const Wire& wire) {
        os << "{ .name=" << wire.name;
        os << ", .hdlname=" << wire.hdlname;
        os << ", .width=" << wire.width;
        os << ", .offset=" << wire.offset;
        return os << " }";
    }

    // Converts a source-level wire into the individual bits that are not already represented by
    // collected flip-flop cells. Yosys' `rename -wire` gives those cells names derived from the
    // wire, so comparing the names remains valid after technology mapping changes their
    // connectivity.
    std::vector<Wire> removeFlipFlopBits(
        const std::unordered_set<std::string>& flip_flop_signals = {}
    ) const;
};
