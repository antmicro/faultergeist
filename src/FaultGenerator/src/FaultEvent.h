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

#include "RandomGen.h"
#include "Signal.h"
#include "UnitUtils.h"

#include <cstdint>
#include <ostream>
#include <random>

struct FaultEventConfig {
    unit::PERCENT latchup_probability;
    unit::PERCENT transient_probability;
    RandomGen& gen;
};

enum class FaultEventType : std::uint8_t {
    UNKNOWN = 0,
    SINGLE_EVENT_TRANSIENT,
    SINGLE_EVENT_UPSET,
    SINGLE_EVENT_LATCHUP,
    AUTO,
};

inline FaultEventType faultEventType(SignalType signal_type, const FaultEventConfig& config) {
    if (signal_type == SignalType::REGISTER) {
        return FaultEventType::SINGLE_EVENT_UPSET;
    }
    std::bernoulli_distribution dist(config.latchup_probability.numerical_value_in(unit::one));
    return dist(config.gen.random_generator) ? FaultEventType::SINGLE_EVENT_LATCHUP
                                             : FaultEventType::SINGLE_EVENT_TRANSIENT;
}

inline std::ostream& operator<<(std::ostream& os, FaultEventType type) {
    switch (type) {
        case FaultEventType::SINGLE_EVENT_TRANSIENT:
            return os << "set";
        case FaultEventType::SINGLE_EVENT_UPSET:
            return os << "seu";
        case FaultEventType::SINGLE_EVENT_LATCHUP:
            return os << "sel";
        default:
            break;
    }
    return os << "unknown";
}

struct FaultEvent {
    Signal::Iterator it;
    unit::SIM_TIME time = 0 * unit::fs;
    std::string_view signal_path;
    std::uint32_t bit_index = 0;
    FaultEventType type = FaultEventType::UNKNOWN;

    bool operator<(const FaultEvent& other) const { return time < other.time; }

    friend std::ostream& operator<<(std::ostream& os, const FaultEvent& ev) {
        os << "{ .time=" << std::format("{}", ev.time);
        os << ", .signal_path=" << ev.signal_path;
        os << ", .bit_index=" << ev.bit_index;
        os << ", .type=" << ev.type;
        return os << " }";
    }
};
