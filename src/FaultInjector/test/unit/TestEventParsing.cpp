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

#include "Event.h"
#include "EventParser.h"

#include <gtest/gtest.h>

namespace {

class EventParserTester : public fin::EventParser {
   public:
    EventParserTester(double multiplier = 1.0) {
        time_multiplier = multiplier;
        insertSignal({"TOP.test_signal", nullptr, 0});
        insertSignal({"TOP.another_sig", nullptr, 0});
        insertSignal({"TOP.sig", nullptr, 0});
    }

    std::optional<fin::Event> parse_line(std::string_view line) { return parse(line); }
};
EventParserTester parser;

TEST(EventParsing, ParsesValidSetEvent) {
    auto result = parser.parse_line("100,TOP.test_signal,3,set");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->time, 100);
    EXPECT_EQ(result->sig_path(), "TOP.test_signal");
    EXPECT_EQ(result->bit_idx, 3);
    EXPECT_EQ(result->type, fin::Event::Type::SingleEventTransientUpset);
    EXPECT_FALSE(result->vpi_value.has_value());
}

TEST(EventParsing, ParsesValidSeuEvent) {
    auto result = parser.parse_line("50,TOP.another_sig,7,seu");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->time, 50);
    EXPECT_EQ(result->sig_path(), "TOP.another_sig");
    EXPECT_EQ(result->bit_idx, 7);
    EXPECT_EQ(result->type, fin::Event::Type::SingleEventUpset);
}

TEST(EventParsing, NonExistantSignalGivesNullopt) {
    std::string_view sig_path;
    auto result = parser.parse_line("100,TOP.null_sig,0,seu");
    EXPECT_FALSE(result.has_value());
}

TEST(EventParsing, ReturnsNulloptForUnknownType) {
    std::string_view sig_path;
    auto result = parser.parse_line("100,TOP.sig,0,unknown");
    EXPECT_FALSE(result.has_value());
}

TEST(EventParsing, ScalesFemtosecondsToDifferentSimulationUnits) {
    struct TestCase {
        double multiplier;
        int expected_ticks;
    };

    for (const auto [multiplier, expected_ticks] : {
             TestCase{1.0, 1'000'000},  // fs
             TestCase{0.001, 1'000},    // ps
             TestCase{0.000001, 1},     // ns
         }) {
        EventParserTester parser{multiplier};
        auto result = parser.parse_line("1000000,TOP.sig,0,seu");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->time, expected_ticks);
    }
}

}  // namespace
