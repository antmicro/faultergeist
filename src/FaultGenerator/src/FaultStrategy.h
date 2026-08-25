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

#include <memory>
#include <random>
#include <span>
#include <vector>

struct Signal;
struct FaultEvent;

class FaultStrategy {
   public:
    struct Config final {
        std::uint64_t num_of_events;
        std::uint32_t seed;
        std::uint64_t simulation_time;
        std::uint32_t thread_number;

        bool tooManyEventsGenerated(std::size_t generated) const {
            return num_of_events > 0 && generated >= num_of_events;
        }

        friend std::ostream& operator<<(std::ostream& os, const Config& config) {
            os << "{ .num_of_events=" << config.num_of_events;
            os << ", .seed=" << config.seed;
            os << ", .simulation_time=" << config.simulation_time;
            os << ", .thread_number=" << config.thread_number;
            return os << " }";
        }
    };
    struct RandomGen final {
        explicit RandomGen(std::uint32_t seed) : random_generator(seed) {}
        std::mt19937 random_generator;
    };

    const Config config;
    RandomGen gen;

    virtual std::vector<FaultEvent> generate(std::span<const Signal> signals) = 0;
    virtual std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) = 0;

   protected:
    FaultStrategy(const Config& config) : config{config}, gen{config.seed} {}
};
