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

#include "OpenROADParser.h"

#include "Placement.h"

#include "LogUtils.h"
#include "UnitUtils.h"

#include <fstream>
#include <string>
#include <vector>

std::optional<CellPlacementInfo> parsePlacement(const std::string& line) {
    std::stringstream is(line);
    std::string name;
    std::getline(is, name, ',');
    std::string type;
    std::getline(is, type, ',');
    std::string width_str;
    std::getline(is, width_str, ',');
    std::string height_str;
    std::getline(is, height_str, ',');
    std::string x_str;
    std::getline(is, x_str, ',');
    std::string y_str;
    std::getline(is, y_str, ',');

    if (name.empty() || type.empty() || width_str.empty() || height_str.empty() || x_str.empty() ||
        y_str.empty()) {
        return std::nullopt;
    }

    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
    try {
        x = std::stod(x_str);
        y = std::stod(y_str);
        width = std::stod(width_str);
        height = std::stod(height_str);
    } catch (...) {
        return std::nullopt;
    }

    const auto cell_placement_scale = 1 * unit::um;
    return CellPlacementInfo{
        .name = name,
        .type = type,
        .placement =
            {.width = width * cell_placement_scale,
             .height = height * cell_placement_scale,
             .x = x * cell_placement_scale,
             .y = y * cell_placement_scale}
    };
}

PlacementInfo OpenROADParser::parse(const std::string& path) {
    std::ifstream file{path};
    SEE_PCHECK(file) << "Failed to open '" << path << "'";

    std::string line;
    std::getline(file, line);
    if (line == "name,type,width,height,x,y") {
        std::getline(file, line);
    }
    auto header = parsePlacement(line);

    std::vector<CellPlacementInfo> result;
    while (std::getline(file, line)) {
        auto info = parsePlacement(line);
        if (!info) {
            LOG(ERROR) << "Failed to parse line: '" << line << "'. Skipping";
            continue;
        }
        result.push_back(*info);
    }
    return {header->placement, result};
}
