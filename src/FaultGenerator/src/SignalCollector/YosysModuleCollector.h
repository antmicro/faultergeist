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

#include <nlohmann/json.hpp>

struct Cell;
struct Module;
class Liberty;

class YosysModuleCollector {
    const Liberty& liberty;

   public:
    YosysModuleCollector(const Liberty& liberty) : liberty(liberty) {}

    std::vector<Module> collectFromFile(const std::filesystem::path&) const;
    std::vector<Module> collect(const nlohmann::json&) const;

   private:
    void collectCell(Module&, Cell, const nlohmann::json&) const;
};
