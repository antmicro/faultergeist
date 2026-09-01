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
#include "PlacementInfo.h"
#include "TestUtils.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <string>

CellPlacementInfo createCellPlacement(
    std::string_view cell_name,
    std::string_view cell_type,
    double width,
    double height,
    double x,
    double y
) {
    return {
        std::string(cell_name),
        std::string(cell_type),
        {width * unit::DIST::unit,
         height * unit::DIST::unit,
         x * unit::DIST::unit,
         y * unit::DIST::unit}
    };
}

std::vector<CellPlacementInfo> expected_cell_placement_info = {
    createCellPlacement("_12_", "INVx1_ASAP7_75t_R", 0.162, 0.27, 1.782, 0),
    createCellPlacement("_13_", "INVx3_ASAP7_75t_R", 0.27, 0.27, 1.728, 0.54),
    createCellPlacement("_14_", "INVx3_ASAP7_75t_R", 0.27, 0.27, 1.458, 1.08),
    createCellPlacement("_15_", "OA21x2_ASAP7_75t_R", 0.378, 0.27, 0, 0),
    createCellPlacement("_16_", "NAND2x2_ASAP7_75t_R", 0.54, 0.27, 0.918, 1.08),
    createCellPlacement("_17_", "OR2x2_ASAP7_75t_R", 0.324, 0.27, 1.188, 1.08),
    createCellPlacement("_18_", "OAI21x1_ASAP7_75t_R", 0.432, 0.27, 0.486, 1.08),
    createCellPlacement("_19_", "AND2x4_ASAP7_75t_R", 0.54, 0.27, 0.648, 1.08),
    createCellPlacement("_20_", "INVx1_ASAP7_75t_R", 0.162, 0.27, 0.054, 0.54),
    createCellPlacement("_21_", "INVx1_ASAP7_75t_R", 0.162, 0.27, 0.216, 0.54),
    createCellPlacement("_22_", "INVx1_ASAP7_75t_R", 0.162, 0.27, 1.782, 0.54),
    createCellPlacement("_23_", "TIEHIx1_ASAP7_75t_R", 0.162, 0.27, 1.026, 1.62),
    createCellPlacement("val\\[0\\]$_DFF_PP1_", "DFFASRHQNx1_ASAP7_75t_R", 1.404, 0.27, 0.378, 0),
    createCellPlacement(
        "val\\[1\\]$_DFF_PP1_",
        "DFFASRHQNx1_ASAP7_75t_R",
        1.404,
        0.27,
        0.378,
        0.54
    ),
    createCellPlacement(
        "val\\[2\\]$_DFF_PP1_",
        "DFFASRHQNx1_ASAP7_75t_R",
        1.404,
        0.27,
        0.324,
        0.54
    ),
};

TEST(OpenROADParsing, ExtractFromExampleCSV) {
    auto filepath = std::string(TEST_DATA_DIR) + "/example.csv.in";

    auto parse_result = OpenROADParser::parse(filepath);

    auto device_info = parse_result.getDeviceInfo();
    ASSERT_TRUE(device_info);
    EXPECT_QUANTITY_DOUBLE_EQ(device_info->height, 2 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(device_info->width, 2 * unit::DIST::unit);

    for (const auto& expected : expected_cell_placement_info) {
        auto actual = parse_result.getCellPlacement(expected.name);
        ASSERT_TRUE(actual);
        EXPECT_QUANTITY_DOUBLE_EQ(actual->height, expected.placement.height);
        EXPECT_QUANTITY_DOUBLE_EQ(actual->width, expected.placement.width);
        EXPECT_QUANTITY_DOUBLE_EQ(actual->x, expected.placement.x);
        EXPECT_QUANTITY_DOUBLE_EQ(actual->y, expected.placement.y);
    }
}
