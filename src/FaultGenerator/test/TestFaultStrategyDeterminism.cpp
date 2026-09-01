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

#include "FaultCampaignWriter.h"
#include "FaultEvent.h"
#include "FaultEventsSignalFormatter.h"
#include "FaultStrategy/Bendel.h"
#include "FaultStrategy/FaultStrategy.h"
#include "FaultStrategy/Random.h"
#include "FaultStrategy/Weibull.h"

#include "TestUtils.h"
#include "UnitUtils.h"

#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <string>
#include <vector>

const FaultStrategy::Config config{
    .num_of_events = 10,
    .seed = 2137,
    .simulation_time = 1000 * unit::s,
    .thread_number = 4
};

const bool OVERRIDE_GOLDENFILES = false;

TEST(FaultGenerationShouldBeDeterministic, WeibullStrategy) {
    WeibullConfig weibull_config = {
        .streams = {
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
        }
    };

    std::vector<Signal> signals = createSignals(64, weibull_config.reference_cell_area, 256 * 1024);
    WeibullStrategy strategy{config, weibull_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/weibull_campaign.csv.out");

    if (OVERRIDE_GOLDENFILES) {
        std::ofstream golden_file{std::string(TEST_DATA_DIR) + "/weibull_campaign.csv.out"};
        FaultCampaignWriter(formatter).write(golden_file, stream_events);
    }

    testStreams(expected, actual);
}

TEST(FaultGenerationShouldBeDeterministic, BendelStrategy) {
    BendelConfig bendel_config = {
        .streams = {
            BendelConfig::Stream{
                .name = "run55",
                .energy = 20.0 * unit::MeV,
                .flux_phi = 1.12e8 * inverse(unit::s * unit::cm2),
                .max_time = 89285714285714288 * unit::fs  // calculated from TestFaultStrategyBendel
            },
            BendelConfig::Stream{
                .name = "run52",
                .energy = 40.0 * unit::MeV,
                .flux_phi = 1.19e8 * inverse(unit::s * unit::cm2),
                .max_time = 84033613445378144 * unit::fs  // calculated from TestFaultStrategyBendel
            },
            BendelConfig::Stream{
                .name = "run47",
                .energy = 60.0 * unit::MeV,
                .flux_phi = 9.17e7 * inverse(unit::s * unit::cm2),
                .max_time =
                    109051254089422032 * unit::fs  // calculated from TestFaultStrategyBendel
            },
        }
    };

    std::vector<Signal> signals = createSignals(64, bendel_config.reference_cell_area, 256 * 1024);
    BendelStrategy strategy{config, bendel_config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/bendel_campaign.csv.out");

    if (OVERRIDE_GOLDENFILES) {
        std::ofstream golden_file{std::string(TEST_DATA_DIR) + "/bendel_campaign.csv.out"};
        FaultCampaignWriter(formatter).write(golden_file, stream_events);
    }

    testStreams(expected, actual);
}

TEST(FaultGenerationShouldBeDeterministic, RandomStrategy) {
    std::vector<Signal> signals = createSignals(10, 10 * unit::um2);

    RandomStrategy strategy{config};
    std::vector<FaultEvent> stream_events = strategy.generate(signals);

    FaultCampaignWriter::FaultFormatter formatter(FaultEventsSignalFormatter("", signals));
    std::stringstream actual;
    FaultCampaignWriter(formatter).write(actual, stream_events);
    std::ifstream expected(std::string(TEST_DATA_DIR) + "/random_campaign.csv.out");

    if (OVERRIDE_GOLDENFILES) {
        std::ofstream golden_file{std::string(TEST_DATA_DIR) + "/random_campaign.csv.out"};
        FaultCampaignWriter(formatter).write(golden_file, stream_events);
    }

    testStreams(expected, actual);
}
