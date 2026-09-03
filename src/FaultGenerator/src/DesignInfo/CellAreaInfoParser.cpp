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

#include "CellAreaInfoParser.h"

#include "Liberty.h"
#include "LogUtils.h"
#include "UnitUtils.h"

#include <fstream>
#include <optional>

namespace {

std::optional<unit::AREA> parseArea(std::string_view area) {
    return unit::parseQuantity(area).and_then([](const auto parsed) {
        auto [value, unit] = parsed;
        return unit::normalizeArea(value, unit);
    });
}

};  // namespace

/*
 * Schema for this json:
 * {
 *      "FF_WITH_AREA": {
 *          "cell_area": "<area_with_units>"
 *          "is_flip_flop": true
 *       },
 *      "NOT_FF_WITH_AREA": {
 *          "cell_area": "<area_with_units>"
 *          "is_flip_flop": false
 *       },
 * }
 * `area_with_units` means a string of a double with units attached.
 * Supported formats are: m2, cm2, mm2, um2, nm2.
 */
LibertyInfo CellAreaJsonParser::parse(std::string_view name, const nlohmann::json& json) {
    LibertyInfo liberty;
    liberty.name = name;

    for (auto [cell_name, cell_json] : json.items()) {
        if (!cell_json.is_object()) {
            continue;
        }
        const auto maybe_area = parseArea(cell_json.value<std::string_view>("cell_area", ""));
        if (!maybe_area) {
            LOG(ERROR) << "Cell " << cell_name
                       << " is missing area description. Skipping that cell.";
            continue;
        }
        bool maybe_ff = false;
        if (cell_json.contains("is_flip_flop")) {
            maybe_ff = cell_json.value("is_flip_flop", false);
        }
        liberty.cells.insert(
            {std::string(cell_name),
             CellInfo{
                 .area = maybe_area,
                 .ff_info = maybe_ff ? std::optional(FlipFlopInfo{}) : std::nullopt
             }}
        );
    }
    return liberty;
}

LibertyInfo CellAreaJsonParser::parse(std::filesystem::path path) {
    std::ifstream cell_area_file(path.c_str());
    SEE_PCHECK(cell_area_file) << "Cannot access cell area config file '" << path << "'";

    try {
        nlohmann::json cell_area_json = nlohmann::json::parse(cell_area_file);
        return parse(path.c_str(), cell_area_json);
    } catch (...) {
        SEE_CHECK(false) << "Malformed cell area config file '" << path << "'";
    }
}
