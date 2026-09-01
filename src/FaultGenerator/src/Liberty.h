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

#include "UnitUtils.h"

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct FlipFlopInfo {
    friend std::ostream& operator<<(std::ostream& os, const FlipFlopInfo&) { return os << "{}"; }
};

struct CellInfo {
    std::optional<unit::AREA> area;
    std::optional<FlipFlopInfo> ff_info;

    friend std::ostream& operator<<(std::ostream& os, const CellInfo& cell) {
        os << "{ .area=";
        if (cell.area) {
            os << std::format("{}", cell.area.value());
        } else {
            os << "nullopt";
        }
        os << ", .ff_info=";
        if (cell.ff_info) {
            os << cell.ff_info.value();
        } else {
            os << "nullopt";
        }
        return os << " }";
    }
};

struct LibertyInfo {
    std::string name;
    std::unordered_map<std::string, CellInfo> cells;
};

class Liberty {
    const std::vector<LibertyInfo> infos;
    std::unordered_set<std::string_view> ff_types;
    std::unordered_map<std::string_view, unit::AREA> cell_areas;

   public:
    Liberty() = default;
    Liberty(unit::AREA area_scale, const std::vector<std::string>&);
    Liberty(const std::vector<LibertyInfo>&);
    bool isFF(std::string_view) const;
    std::optional<unit::AREA> getArea(std::string_view) const;
    bool contains(const std::string& cell_type) const;
};
