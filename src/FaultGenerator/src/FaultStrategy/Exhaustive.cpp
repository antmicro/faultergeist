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

#include "Exhaustive.h"

ExhaustiveStrategy::ExhaustiveStrategy(const Config& config) : FaultStrategy(config) {}

std::vector<FaultEvent> ExhaustiveStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> fault_events;
    fault_events.reserve(signals.size() * 2);

    for (size_t i = 0; i < signals.size(); ++i) {
        const auto& signal = signals[i];
        for (std::uint32_t width = 0; width < signal.cell.width; ++width) {
            fault_events.emplace_back(FaultEvent{
                signals.begin() + i,
                0 * config.simulation_time.unit,
                /*signal_path=*/"",
                width,
                FaultEventType::SINGLE_EVENT_UPSET
            });
        }
    }
    for (size_t i = 0; i < signals.size(); ++i) {
        const auto& signal = signals[i];
        for (std::uint32_t width = 0; width < signal.cell.width; ++width) {
            fault_events.emplace_back(FaultEvent{
                signals.begin() + i,
                1 * config.simulation_time,
                /*signal_path=*/"",
                width,
                FaultEventType::SINGLE_EVENT_UPSET
            });
        }
    }
    return fault_events;
}

std::shared_ptr<FaultStrategy> ExhaustiveStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<ExhaustiveStrategy>(new_config);
}
