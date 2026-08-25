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

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "FaultStrategyBendel.h"
#include "FaultStrategyWeibull.h"

#include <gtest/gtest.h>

namespace {

const double DEFAULT_CELL_AREA = 0.8925 * 1e-6;  // [um^2]

std::vector<Signal> createSignals(std::size_t count) {
    std::vector<Signal> signals;
    signals.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
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

}  // anonymous namespace

TEST(BendelGenerationTest, CountsWithinTolerance) {
    // From bendel_fit.py
    constexpr std::array expected_counts = {344, 621, 862};
    constexpr std::uint64_t expected_total = 1645;
    constexpr double stream_tolerance = 0.35;
    constexpr double total_tolerance = 0.05;
    constexpr std::size_t samples = 10;

    std::vector<Signal> signals = createSignals(100);

    FaultStrategy::Config config{
        .num_of_events = 0,
        .seed = 42,
        .simulation_time = 9999999,
        .thread_number = 1,
    };

    std::vector<BendelConfig::Stream> streams = {
        BendelConfig::Stream{
            .name = "run55", .energy = 20.0 * 1e6, .flux_phi = 1.12e8 * 1e4, .fluence = 1e10 * 1e4
        },
        BendelConfig::Stream{
            .name = "run52", .energy = 40.0 * 1e6, .flux_phi = 1.19e8 * 1e4, .fluence = 1e10 * 1e4
        },
        BendelConfig::Stream{
            .name = "run47", .energy = 60.0 * 1e6, .flux_phi = 9.17e7 * 1e4, .fluence = 1e10 * 1e4
        },
    };

    std::vector<FaultEvent> all_events;
    for (std::size_t i = 0; i < expected_counts.size(); ++i) {
        BendelStrategy strategy{config, BendelConfig{{streams[i]}}};
        std::vector<FaultEvent> stream_events = strategy.generate(signals);
        std::uint64_t count = stream_events.size();
        std::uint64_t expected = expected_counts[i];
        double diff =
            std::abs(static_cast<double>(count) - static_cast<double>(expected)) / expected;

        EXPECT_LE(diff, stream_tolerance) << "stream #" << i << ": got " << count << ", expected "
                                          << expected << " (" << diff * 100.0 << "% diff)";
        all_events.insert(all_events.end(), stream_events.begin(), stream_events.end());
    }

    double total_diff =
        std::abs(static_cast<double>(all_events.size()) - static_cast<double>(expected_total)) /
        expected_total;
    EXPECT_LE(total_diff, total_tolerance)
        << "total events " << all_events.size() << ", expected " << expected_total << " ("
        << total_diff * 100.0 << "% diff)";
}

TEST(BendelGenerationTest, WhenInParallelResultIsSorted) {
    std::vector<Signal> signals = createSignals(100);

    FaultStrategy::Config config{
        .num_of_events = 0,
        .seed = 42,
        .simulation_time = 9999999,
        .thread_number = 4u,
    };

    BendelConfig bendel_config =
        {.streams = {
             BendelConfig::Stream{
                 .name = "run55",
                 .energy = 20.0 * 1e6,
                 .flux_phi = 1.12e8 * 1e4,
                 .fluence = 1e10 * 1e4
             },
             BendelConfig::Stream{
                 .name = "run52",
                 .energy = 40.0 * 1e6,
                 .flux_phi = 1.19e8 * 1e4,
                 .fluence = 1e10 * 1e4
             },
             BendelConfig::Stream{
                 .name = "run47",
                 .energy = 60.0 * 1e6,
                 .flux_phi = 9.17e7 * 1e4,
                 .fluence = 1e10 * 1e4
             },
         }};

    BendelStrategy strategy{config, bendel_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    ASSERT_TRUE(std::is_sorted(stream_events.begin(), stream_events.end()));
}
