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

[[maybe_unused]]
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

[[maybe_unused]]
static Signal createSignal(
    std::string prefix_path,
    std::string signal_name,
    std::uint32_t width,
    std::string hdlname = "",
    unit::AREA cell_area = 1.0 * unit::AREA::unit,
    SignalType signal_type = SignalType::REGISTER
) {
    return Signal(
        {
            .name = signal_name,
            .type = signal_type == SignalType::REGISTER ? "$dff" : "wire",
            .hdlname = std::move(hdlname),
            .width = width,
        },
        std::move(prefix_path),
        signal_type == SignalType::REGISTER ? cell_area : 0.0 * unit::AREA::unit,
        std::nullopt,
        signal_type
    );
}

[[maybe_unused]]
static std::vector<Signal> createSignals(
    size_t count,
    unit::AREA cell_area,
    std::uint32_t cell_width = 1024
) {
    std::vector<Signal> signals;
    signals.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string signal_name = "signal_" + std::to_string(i);
        signals.push_back(createSignal(
            /*prefix_path=*/"",
            std::move(signal_name),
            cell_width,
            /*hdlname=*/"",
            cell_area,
            SignalType::REGISTER
        ));
    }
    return signals;
}

// Creates an array of registers with 50/50 ratio.
[[maybe_unused]]
static std::vector<Signal> createSignalsWithWires(
    size_t count,
    unit::AREA cell_area,
    std::uint32_t cell_width = 1024
) {
    std::vector<Signal> signals;
    signals.reserve(count);
    size_t i = 0;
    for (; i < count / 2; ++i) {
        std::string signal_name = "signal_" + std::to_string(i);
        signals.push_back(createSignal(
            /*prefix_path=*/"",
            std::move(signal_name),
            cell_width,
            /*hdlname=*/"",
            cell_area,
            SignalType::REGISTER
        ));
    }
    for (; i < count; ++i) {
        std::string signal_name = "wire_" + std::to_string(i);
        signals.push_back(createSignal(
            /*prefix_path=*/"",
            std::move(signal_name),
            cell_width,
            /*hdlname=*/"",
            cell_area,
            SignalType::WIRE
        ));
    }
    return signals;
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
