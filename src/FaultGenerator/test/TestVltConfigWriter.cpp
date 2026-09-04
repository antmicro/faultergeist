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

#include "FaultEventsSignalFormatter.h"
#include "Signal.h"
#include "TestUtils.h"
#include "VltConfigWriter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <regex>
#include <sstream>

TEST(VltConfigWriter, PrintHeader) {
    std::stringstream actual;

    VltConfigWriter::write(actual, {});
    std::string printout = actual.str();
    EXPECT_THAT(printout, ::testing::ContainsRegex("`verilator_config"));
}

TEST(VltConfigWriter, PrintsRegisterSignals) {
    const std::vector<Signal> signals = {
        createSignal("top.worker", "counter", 32),
        createSignal("top.worker", "resp", 32),
    };
    std::stringstream actual;

    VltConfigWriter::write(actual, signals);
    std::string printout = actual.str();

    EXPECT_THAT(printout, ::testing::ContainsRegex("public_flat_rw.*\\*.*counter"));
    EXPECT_THAT(printout, ::testing::ContainsRegex("public_flat_rw.*\\*.*resp"));
    EXPECT_THAT(printout, ::testing::Not(::testing::HasSubstr("forceable")));
}

TEST(VltConfigWriter, PrintsWireSignals) {
    std::vector<Signal> signals = {
        createSignal("top.worker", "request", 1),
        createSignal("top.worker", "response", 1),
    };
    signals[0].type = SignalType::WIRE;
    signals[1].type = SignalType::WIRE;
    std::stringstream actual;

    VltConfigWriter::write(actual, signals);
    std::string printout = actual.str();

    EXPECT_THAT(printout, ::testing::ContainsRegex("forceable.*\\*.*request"));
    EXPECT_THAT(printout, ::testing::ContainsRegex("forceable.*\\*.*response"));
    EXPECT_THAT(printout, ::testing::Not(::testing::HasSubstr("public_flat_rw")));
}

TEST(VltConfigWriter, DoesNotPrintUnknownSignals) {
    auto signal = createSignal("top.worker", "counter", 32);
    signal.type = SignalType::UNKNOWN;
    std::stringstream actual;

    VltConfigWriter::write(actual, std::span<const Signal>{&signal, 1});

    EXPECT_THAT(actual.str(), ::testing::Not(::testing::HasSubstr("-module")));
}

TEST(VltConfigWriter, DoesNotDuplicateRegisterOrWireSignals) {
    std::vector<Signal> signals = {
        createSignal("top.first", "counter", 32),
        createSignal("top.second", "counter", 32),
        createSignal("top.first", "response", 1),
        createSignal("top.second", "response", 1),
    };
    signals[2].type = SignalType::WIRE;
    signals[3].type = SignalType::WIRE;
    std::stringstream actual;

    VltConfigWriter::write(actual, signals);
    std::string printout = actual.str();

    const std::regex register_pattern{"public_flat_rw.*\\*.*counter"};
    const std::regex wire_pattern{"forceable.*\\*.*response"};
    EXPECT_EQ(
        std::distance(
            std::sregex_iterator(printout.begin(), printout.end(), register_pattern),
            std::sregex_iterator{}
        ),
        1
    );
    EXPECT_EQ(
        std::distance(
            std::sregex_iterator(printout.begin(), printout.end(), wire_pattern),
            std::sregex_iterator{}
        ),
        1
    );
}

TEST(VltConfigWriter, RemovesIndicesAndDuplicatesFromSynthesizedRegisterSignals) {
    std::vector<Signal> signals;
    for (std::size_t bit = 0; bit < 32; ++bit) {
        signals.push_back(createSignal(
            "top.worker", "dff_worker0.counter[" + std::to_string(bit) + "]$_DFFE_PP_", 1
        ));
        signals.push_back(createSignal(
            "top.worker", "dff_worker0.response[" + std::to_string(bit) + "]$_DFFE_PP_", 1
        ));
    }
    std::stringstream actual;

    VltConfigWriter::write(actual, signals);
    std::string printout = actual.str();

    const std::regex counter_pattern{"public_flat_rw.*\\*.*counter"};
    const std::regex response_pattern{"public_flat_rw.*\\*.*response"};
    EXPECT_THAT(printout, ::testing::Not(::testing::HasSubstr("[")));
    EXPECT_EQ(
        std::distance(
            std::sregex_iterator(printout.begin(), printout.end(), counter_pattern),
            std::sregex_iterator{}
        ),
        1
    );
    EXPECT_EQ(
        std::distance(
            std::sregex_iterator(printout.begin(), printout.end(), response_pattern),
            std::sregex_iterator{}
        ),
        1
    );
}
