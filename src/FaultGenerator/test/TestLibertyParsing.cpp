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

#include "LibertyParser.h"
#include "TestUtils.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <string>

TEST(LibParsing, ExtractFromExampleLib) {
    auto filepath = std::string(TEST_DATA_DIR) + "/example.lib";

    auto parser = LibertyParser(1 * unit::um2);
    auto parse_result = parser.parse(filepath);
    ASSERT_TRUE(parse_result);

    const auto actual = *parse_result;

    // Cell DVBX1
    ASSERT_TRUE(actual.cells.contains("DCBX1"));
    ASSERT_TRUE(actual.cells.at("DCBX1").area);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.cells.at("DCBX1").area.value(), 18.0001 * unit::um2);

    // Cell INVX1
    ASSERT_TRUE(actual.cells.contains("INVX1"));
    ASSERT_TRUE(actual.cells.at("INVX1").area);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.cells.at("INVX1").area.value(), 3 * unit::um2);

    // Cell NAND2X1
    ASSERT_TRUE(actual.cells.contains("NAND2X1"));
    ASSERT_TRUE(actual.cells.at("NAND2X1").area);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.cells.at("NAND2X1").area.value(), 4 * unit::um2);
}
