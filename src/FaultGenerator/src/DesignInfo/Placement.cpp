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

#include "OpenROADParser.h"

PlacementInfo::PlacementInfo(
    std::optional<Placement> device_info,
    std::vector<CellPlacementInfo> cell_info
)
    : device_info(std::move(device_info)), cell_info(std::move(cell_info)) {}

PlacementInfo::PlacementInfo(const std::string& filepath)
    : PlacementInfo(OpenROADParser::parse(filepath)) {}

std::optional<Placement> PlacementInfo::getCellPlacement(const std::string& cell_name) const {
    for (const auto& info : cell_info) {
        if (info.name == cell_name) {
            return info.placement;
        }
    }
    return std::nullopt;
}
