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

#include "Signal.h"

#include <gtest/gtest.h>

#include <istream>

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

const double DEFAULT_CELL_AREA = 0.25 * 1e-6;  // [cm^2]

static std::vector<Signal> createSignals(size_t count) {
    std::vector<Signal> signals;
    signals.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        std::string signal_name = "signal_" + std::to_string(i);
        signals.push_back(Signal{
            Cell{.name = signal_name, .width = 1024},
            /*prefix_path=*/"",
            DEFAULT_CELL_AREA,
            std::nullopt,
            SignalType::REGISTER
        });
    }
    return signals;
}
