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

#include "Cell.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct Module {
    std::string name;

    // Name of the module instance and index to the module inside modules array.
    std::vector<std::pair<std::string, unsigned int>> child_modules;

    std::vector<Cell> cells;

    std::string dump() const {
        std::stringstream ss;
        ss << "Module: " << name << "\n";
        ss << "  Signals:\n";
        for (const auto& cell : cells) {
            ss << "    " << cell << "\n";
        }
        for (const auto& [instance_name, module_index] : child_modules) {
            ss << "  Child modules:\n";
            ss << "    { " << instance_name << ": " << name << " }\n";
        }
        return ss.str();
    }
};
