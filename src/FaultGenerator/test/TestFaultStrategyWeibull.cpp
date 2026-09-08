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

#include "FaultEvent.h"
#include "FaultStrategy/Weibull.h"
#include "TestUtils.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {

static const unit::AREA DEFAULT_CELL_AREA = 0.25 * 1e-6 * unit::cm2;  // [cm^2]

}  // anonymous namespace

TEST(WeibullGenerationTest, CountsWithinTolerance) {
    // From seu2.py
    constexpr std::array expected_counts = {2431964, 245205, 103683, 101457, 20256,  99086, 102352,
                                            104023,  97811,  2741,   91813,  98614,  52001, 90941,
                                            9825,    9659,   25,     26,     160635, 10396};
    constexpr std::uint64_t expected_total =
        std::accumulate(expected_counts.begin(), expected_counts.end(), 0);

    std::vector<Signal> signals =
        createSignals(4 * 1024, DEFAULT_CELL_AREA, 1024);  // total 4Mbit as in the experiment

    FaultStrategy::Config config{
        .num_of_events = 0,
        .seed = 42,
        .simulation_time = 9999999 * unit::s,
        .thread_number = 1,
    };

    std::vector<WeibullConfig::Stream> streams = {
        WeibullConfig::Stream{
            .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 9.15e3 * inverse(unit::s * unit::cm2),
            .max_time = 1094 * unit::s
        },
        WeibullConfig::Stream{
            .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
            .max_time = 996 * unit::s
        },
        WeibullConfig::Stream{
            .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.04e3 * inverse(unit::s * unit::cm2),
            .max_time = 409 * unit::s
        },
        WeibullConfig::Stream{
            .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.05e3 * inverse(unit::s * unit::cm2),
            .max_time = 399 * unit::s
        },
        WeibullConfig::Stream{
            .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 5.04e2 * inverse(unit::s * unit::cm2),
            .max_time = 166 * unit::s
        },
        WeibullConfig::Stream{
            .let = 40.4 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
            .max_time = 536 * unit::s
        },
        WeibullConfig::Stream{
            .let = 40.4 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
            .max_time = 551 * unit::s
        },
        WeibullConfig::Stream{
            .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.58e3 * inverse(unit::s * unit::cm2),
            .max_time = 417 * unit::s
        },
        WeibullConfig::Stream{
            .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.51e3 * inverse(unit::s * unit::cm2),
            .max_time = 411 * unit::s
        },
        WeibullConfig::Stream{
            .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 1.45e3 * inverse(unit::s * unit::cm2),
            .max_time = 12 * unit::s
        },
        WeibullConfig::Stream{
            .let = 20.4 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 2.00e3 * inverse(unit::s * unit::cm2),
            .max_time = 433 * unit::s
        },
        WeibullConfig::Stream{
            .let = 20.4 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 2.05e3 * inverse(unit::s * unit::cm2),
            .max_time = 452 * unit::s
        },
        WeibullConfig::Stream{
            .let = 10.2 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 2.32e3 * inverse(unit::s * unit::cm2),
            .max_time = 433 * unit::s
        },
        WeibullConfig::Stream{
            .let = 10.2 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 2.77e3 * inverse(unit::s * unit::cm2),
            .max_time = 636 * unit::s
        },
        WeibullConfig::Stream{
            .let = 3.0 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 5.03e3 * inverse(unit::s * unit::cm2),
            .max_time = 201 * unit::s
        },
        WeibullConfig::Stream{
            .let = 3.0 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 5.11e3 * inverse(unit::s * unit::cm2),
            .max_time = 197 * unit::s
        },
        WeibullConfig::Stream{
            .let = 1.1 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 7.60e3 * inverse(unit::s * unit::cm2),
            .max_time = 133 * unit::s
        },
        WeibullConfig::Stream{
            .let = 1.1 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 8.17e3 * inverse(unit::s * unit::cm2),
            .max_time = 124 * unit::s
        },
        WeibullConfig::Stream{
            .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 9.99e3 * inverse(unit::s * unit::cm2),
            .max_time = 102 * unit::s
        },
        WeibullConfig::Stream{
            .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
            .flux_phi = 5.17e1 * inverse(unit::s * unit::cm2),
            .max_time = 1275 * unit::s
        }
    };

    std::vector<FaultEvent> all_events;

    // GTEST_SKIP() << "This will require fine tuning";

    for (std::size_t i = 0; i < expected_counts.size(); ++i) {
        WeibullStrategy strategy{config, WeibullConfig{{streams[i]}}};
        std::vector<FaultEvent> stream_events = strategy.generate(signals);
        std::uint64_t count = stream_events.size();
        std::uint64_t expected = expected_counts[i];
        all_events.insert(all_events.end(), stream_events.begin(), stream_events.end());

        double tolerance;
        if (expected > 100000) {
            tolerance = 0.05;
        } else if (expected > 1000) {
            tolerance = 0.10;
        } else if (expected > 100) {
            tolerance = 0.20;
        } else {
            tolerance = 5.0;
        }

        double diff = (static_cast<double>(count) - static_cast<double>(expected)) / expected;
        double abs_diff = std::abs(diff);
        std::cerr << "\nstream #" << i << ": got " << count << ", expected " << expected << " ("
                  << diff * 100.0 << "% diff)\n";
        EXPECT_LE(abs_diff, tolerance) << "stream #" << i << ": got " << count << ", expected "
                                       << expected << " (" << abs_diff * 100.0 << "% diff)";
    }

    double total_diff =
        std::abs(static_cast<double>(all_events.size()) - static_cast<double>(expected_total)) /
        expected_total;
    EXPECT_LE(total_diff, 0.05) << "total events " << all_events.size() << ", expected "
                                << expected_total << " (" << total_diff * 100.0 << "% diff)";
}

TEST(WeibullGenerationTest, WhenInParallelResultIsSorted) {
    std::vector<Signal> signals = createSignals(10, 10 * unit::um2, 10);

    FaultStrategy::Config config{
        .num_of_events = 0,
        .seed = 42,
        .simulation_time = 9999999 * unit::s,
        .thread_number = 4u,
    };

    WeibullConfig weibull_config =
        {.streams = {
             WeibullConfig::Stream{
                 .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 9.15e3 * inverse(unit::s * unit::cm2),
                 .max_time = 1094 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
                 .max_time = 996 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.04e3 * inverse(unit::s * unit::cm2),
                 .max_time = 409 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.05e3 * inverse(unit::s * unit::cm2),
                 .max_time = 399 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 67.7 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 5.04e2 * inverse(unit::s * unit::cm2),
                 .max_time = 166 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 40.4 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
                 .max_time = 536 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 40.4 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.01e3 * inverse(unit::s * unit::cm2),
                 .max_time = 551 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.58e3 * inverse(unit::s * unit::cm2),
                 .max_time = 417 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.51e3 * inverse(unit::s * unit::cm2),
                 .max_time = 411 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 1.45e3 * inverse(unit::s * unit::cm2),
                 .max_time = 12 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 20.4 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 2.00e3 * inverse(unit::s * unit::cm2),
                 .max_time = 433 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 20.4 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 2.05e3 * inverse(unit::s * unit::cm2),
                 .max_time = 452 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 10.2 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 2.32e3 * inverse(unit::s * unit::cm2),
                 .max_time = 433 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 10.2 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 2.77e3 * inverse(unit::s * unit::cm2),
                 .max_time = 636 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 3.0 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 5.03e3 * inverse(unit::s * unit::cm2),
                 .max_time = 201 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 3.0 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 5.11e3 * inverse(unit::s * unit::cm2),
                 .max_time = 197 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 1.1 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 7.60e3 * inverse(unit::s * unit::cm2),
                 .max_time = 133 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 1.1 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 8.17e3 * inverse(unit::s * unit::cm2),
                 .max_time = 124 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 9.99e3 * inverse(unit::s * unit::cm2),
                 .max_time = 102 * unit::s
             },
             WeibullConfig::Stream{
                 .let = 32.6 * unit::MeV * unit::cm2 / unit::mg,
                 .flux_phi = 5.17e1 * inverse(unit::s * unit::cm2),
                 .max_time = 1275 * unit::s
             }
         }};

    WeibullStrategy strategy{config, weibull_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    ASSERT_TRUE(std::is_sorted(stream_events.begin(), stream_events.end()));
}
