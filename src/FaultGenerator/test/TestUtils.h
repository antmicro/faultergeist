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

#include "FaultEvent.h"
#include "Signal.h"
#include "UnitUtils.h"
#include "Utils.h"

#include <gtest/gtest.h>

#include <istream>

#define EXPECT_QUANTITY_DOUBLE_EQ(actual, expected) \
    EXPECT_DOUBLE_EQ( \
        (actual).numerical_value_in((expected).unit), \
        (expected).numerical_value_in((expected).unit) \
    )

static void testStreams(std::istream& s1, std::istream& s2) {
    int counter = 0;
    while (counter++ < 20) {
        std::string pre_str;
        std::string post_str;
        bool pre_getline = (bool)std::getline(s1, pre_str);
        bool post_getline = (bool)std::getline(s2, post_str);
        ASSERT_EQ(pre_getline, post_getline);
        if (!pre_getline) {
            break;
        }
        EXPECT_EQ(pre_str, post_str);
    }
}

static std::vector<Signal> createSignals(
    size_t count,
    unit::AREA cell_area,
    std::uint32_t cell_width = 1024
) {
    std::vector<Signal> signals;
    signals.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string signal_name = "signal_" + std::to_string(i);
        signals.push_back(
            Signal{
                Cell{.name = signal_name, .width = cell_width},
                /*prefix_path=*/"",
                cell_area,
                std::nullopt,
                SignalType::REGISTER
            }
        );
    }
    return signals;
}

[[maybe_unused]]
static Signal createSignal(
    std::string prefix_path,
    std::string signal_name,
    std::uint32_t width,
    std::string hdlname = ""
) {
    constexpr unit::AREA DEFAULT_AREA =
        1.0 * unit::AREA::unit;  // this is not important to this module

    return Signal(
        {
            .name = signal_name,
            .type = "$dff",
            .hdlname = std::move(hdlname),
            .width = width,
        },
        std::move(prefix_path),
        DEFAULT_AREA,
        std::nullopt,
        SignalType::REGISTER
    );
}

[[maybe_unused]]
static FaultEvent createFromSignal(
    std::span<const Signal> signals,
    std::size_t id,
    std::size_t time,
    std::uint32_t bit
) {
    const auto& signal = signals[id];
    return FaultEvent{
        signals.begin() + id,
        time * unit::SIM_TIME::unit,
        combineSignalPath(signal.path_prefix, signal.cell.getPath()),
        bit,
        FaultEventType::SINGLE_EVENT_UPSET
    };
}
