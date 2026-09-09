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

#include "Placement.h"

#include "PlacementParser.h"

#include <algorithm>
#include <string>
#include <utility>

PlacementInfo::PlacementInfo(
    std::optional<Placement> device_info,
    std::vector<CellPlacementInfo> cell_info
)
    : device_info(std::move(device_info)) {
    this->cell_info.reserve(cell_info.size());
    for (auto& info : cell_info) {
        // OpenROAD escapes Verilog instance names
        std::erase(info.name, '\\');
        this->cell_info.emplace(std::move(info.name), info.placement);
    }
}

PlacementInfo::PlacementInfo(const std::string& filepath)
    : PlacementInfo(PlacementParser::parse(filepath)) {}

std::optional<Placement> PlacementInfo::getCellPlacement(const std::string& cell_name) const {
    auto found = cell_info.find(cell_name);
    if (found == cell_info.end()) {
        return std::nullopt;
    }
    return found->second;
}
