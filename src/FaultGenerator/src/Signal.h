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

#include "PlacementInfo.h"
#include "SignalCollector/Cell.h"
#include "UnitUtils.h"
#include "Utils.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <span>
#include <string>

enum class SignalType {
    UNKNOWN = 0,
    WIRE,
    REGISTER,
};

inline std::ostream& operator<<(std::ostream& os, const SignalType& type) {
    switch (type) {
        case SignalType::REGISTER:
            return os << "register";
        case SignalType::WIRE:
            return os << "wire";
        default:
            return os << "unknown";
    }
}

struct Signal {
    using Iterator = std::span<const Signal>::iterator;

    const Cell cell;
    std::string path_prefix;
    unit::AREA area;
    std::optional<Placement> cell_placement;
    SignalType type = SignalType::UNKNOWN;

    friend std::ostream& operator<<(std::ostream& os, const Signal& signal) {
        os << "{ .path_prefix=" << signal.path_prefix;
        os << ", .cell=" << signal.cell;
        os << ", .area=" << std::format("{}", signal.area);
        os << ", .placement=";
        if (signal.cell_placement) {
            os << signal.cell_placement.value();
        } else {
            os << "nullopt";
        }
        os << ", .type=" << signal.type;
        return os << " }";
    }
};
