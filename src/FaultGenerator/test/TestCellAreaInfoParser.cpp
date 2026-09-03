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

#include "DesignInfo/CellAreaInfoParser.h"
#include "DesignInfo/Liberty.h"
#include "TestUtils.h"

#include <gtest/gtest.h>

using namespace nlohmann::literals;

TEST(CellAreaJsonParserTest, ParsesPresentFieldsAndLeavesMissingFieldsEmpty) {
    const auto json = R"({
        "complete": {"cell_area": "1.5m2", "is_flip_flop": true},
        "area_only": {"cell_area": "2mm2"},
        "flip_flop_only": {"is_flip_flop": true},
        "empty": {}
    })"_json;

    const auto actual = CellAreaJsonParser::parse("test", json);

    EXPECT_EQ(actual.name, "test");
    ASSERT_EQ(actual.cells.size(), 2);

    ASSERT_TRUE(actual.cells.at("complete").area);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.cells.at("complete").area.value(), 1.5 * unit::m2);
    EXPECT_TRUE(actual.cells.at("complete").ff_info);

    ASSERT_TRUE(actual.cells.at("area_only").area);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.cells.at("area_only").area.value(), 2 * unit::mm2);
    EXPECT_FALSE(actual.cells.at("area_only").ff_info);

    EXPECT_FALSE(actual.cells.contains("flip_flop_only"));
    EXPECT_FALSE(actual.cells.contains("empty"));
}
