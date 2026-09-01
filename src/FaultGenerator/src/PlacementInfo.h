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
#include <vector>

struct Placement {
    unit::DIST width;
    unit::DIST height;
    unit::DIST x;
    unit::DIST y;

    friend std::ostream& operator<<(std::ostream& os, const Placement& pos) {
        return os << "{ "
                  << std::format(
                         ".width={}, .height={}, .x={}, .y={}", pos.width, pos.height, pos.x, pos.y
                     )
                  << " }";
    }
};

struct CellPlacementInfo {
    std::string name;
    std::string type;
    Placement placement;

    friend std::ostream& operator<<(std::ostream& os, const CellPlacementInfo& info) {
        return os << "{ .name=" << info.name << ", .type=" << info.type
                  << ", .position=" << info.placement << " }";
    }
};

class PlacementInfo {
    std::optional<Placement> device_info;
    std::vector<CellPlacementInfo> cell_info;

   public:
    PlacementInfo() = default;
    PlacementInfo(const std::string&);
    PlacementInfo(std::optional<Placement>, std::vector<CellPlacementInfo>);
    const std::optional<Placement>& getDeviceInfo() const { return device_info; }

    std::optional<Placement> getCellPlacement(const std::string& cell_name) const;
};
