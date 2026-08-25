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

#include <ostream>

struct Cell {
    std::string name;
    std::string type;
    std::string hdlname;
    std::uint32_t width;

    /* Returns signal path, in current (yosys dependent) implementation,
     * by taking prefix before first '$'.
     */
    inline std::string getPath() const { return name.substr(0, name.find('$')); }

    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
        os << "{ .name=" << cell.name;
        os << ", .type=" << cell.type;
        os << ", .hdlname=" << cell.hdlname;
        os << ", .width=" << cell.width;
        return os << " }";
    }
};
