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

#include "Liberty.h"

#include "LibertyParser.h"

#include <absl/log/check.h>
#include <absl/log/log.h>

#include <string>
#include <string_view>
#include <vector>

/*****************************************************************************/

Liberty::Liberty(const std::vector<LibertyInfo>& infos_) : infos(infos_) {
    for (const auto& infos : infos) {
        for (const auto& [cell_type, cell_info] : infos.cells) {
            // Register ff_types
            if (cell_info.ff_info) {
                ff_types.insert(cell_type);
            }

            // Register cell_areas
            if (cell_info.area) {
                cell_areas[cell_type] = cell_info.area.value();
            }
        }
    }
}

Liberty::Liberty(const std::vector<std::string>& filepaths)
    : Liberty(LibertyParser::parseFiles(filepaths)) {}

bool Liberty::isFF(std::string_view type) const {
    return ff_types.contains(type);
}

std::optional<double> Liberty::getArea(std::string_view type) const {
    if (auto res = cell_areas.find(type); res != cell_areas.end()) {
        return res->second;
    } else {
        return std::nullopt;
    }
}

bool Liberty::contains(const std::string& type) const {
    return std::any_of(infos.begin(), infos.end(), [&](const LibertyInfo& info) {
        return info.cells.contains(std::string(type));
    });
}
