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
#include "LogUtils.h"

ExhaustiveStrategy::ExhaustiveStrategy(
    const Config& config,
    const ExhaustiveConfig& exhaustive_config
)
    : FaultStrategy(config), exhaustive_config(exhaustive_config) {}

namespace {
void populateFaults(
    std::vector<FaultEvent>& fault_events,
    std::span<const Signal> signals,
    FaultEventType type,
    unit::SIM_TIME time
) {
    for (size_t i = 0; i < signals.size(); ++i) {
        const auto& signal = signals[i];
        for (std::uint32_t width = 0; width < signal.cell.width; ++width) {
            fault_events.emplace_back(FaultEvent{
                signals.begin() + i,
                time,
                /*signal_path=*/"",
                width,
                type
            });
        }
    }
}
}  // namespace

std::vector<FaultEvent> ExhaustiveStrategy::generate(std::span<const Signal> signals) {
    std::vector<FaultEvent> fault_events;

    switch (exhaustive_config.fault_type) {
        case FaultEventType::SINGLE_EVENT_UPSET:
            fault_events.reserve(signals.size() * 2);
            populateFaults(
                fault_events, signals, exhaustive_config.fault_type, 0 * config.simulation_time
            );
            populateFaults(
                fault_events, signals, exhaustive_config.fault_type, 1 * config.simulation_time
            );
            break;
        case FaultEventType::SINGLE_EVENT_TRANSIENT:
            fault_events.reserve(signals.size());
            populateFaults(
                fault_events, signals, exhaustive_config.fault_type, 0 * config.simulation_time
            );
            break;
        default:
            PLOG(ERROR) << "Unknown fault event type: " << exhaustive_config.fault_type;
    }
    return fault_events;
}

std::shared_ptr<FaultStrategy> ExhaustiveStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<ExhaustiveStrategy>(new_config, exhaustive_config);
}
