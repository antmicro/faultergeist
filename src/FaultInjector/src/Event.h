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

#include "Signal.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <type_traits>

#include "vpi_user.h"

namespace fin {

struct Event {
    enum class Type : std::uint8_t {
        SingleEventTransientUpset,
        SingleEventUpset,
    };

    const Signal* signal = nullptr;
    std::uint64_t time{};
    int bit_idx{};
    Type type{};
    std::optional<s_vpi_value> vpi_value{};

    bool operator<(const Event& other) const { return time < other.time; }

    friend std::ostream& operator<<(std::ostream& os, const Event& ev) {
        return os << "{[" << ev.time << "] " << ev.signal->path << "(" << ev.bit_idx << ")}";
    }

    vpiHandle handle() const { return signal->vpi_handle.handle(); }
    std::string_view sig_path() const { return signal->path; }
};

static_assert(std::is_trivially_destructible_v<Event>);

}  // namespace fin
