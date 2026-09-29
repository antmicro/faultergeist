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

std::vector<FaultEvent> ExhaustiveStrategy::generate(
    const MBUGenerator&,
    std::span<const Signal> signals
) {
    std::vector<FaultEvent> fault_events;
    RandomGen gen(config.seed);

    switch (exhaustive_config.fault_type) {
        case FaultEventType::AUTO:
            fault_events.reserve(signals.size() * 2);
            for (std::size_t i = 0; i < signals.size(); ++i) {
                const auto& signal = signals[i];
                const auto type = faultEventType(
                    signal.type, {config.latchup_probability, config.transient_probability, gen}
                );
                for (std::uint32_t bit = 0; bit < signal.cell.width; ++bit) {
                    fault_events.emplace_back(FaultEvent{
                        signals.begin() + i,
                        0 * config.simulation_time,
                        /*signal_path=*/"",
                        bit,
                        type
                    });
                }
            }
            for (std::size_t i = 0; i < signals.size(); ++i) {
                const auto& signal = signals[i];
                if (faultEventType(
                        signal.type, {config.latchup_probability, config.transient_probability, gen}
                    ) != FaultEventType::SINGLE_EVENT_UPSET) {
                    continue;
                }
                for (std::uint32_t bit = 0; bit < signal.cell.width; ++bit) {
                    fault_events.emplace_back(FaultEvent{
                        signals.begin() + i,
                        1 * config.simulation_time,
                        /*signal_path=*/"",
                        bit,
                        FaultEventType::SINGLE_EVENT_UPSET
                    });
                }
            }
            break;
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
            LOG(WARNING) << "Unknown requested fault type in exhaustive strategy!";
    }
    return fault_events;
}

std::shared_ptr<FaultStrategy> ExhaustiveStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<ExhaustiveStrategy>(new_config, exhaustive_config);
}
