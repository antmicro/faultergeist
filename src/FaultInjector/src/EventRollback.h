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
#include <memory>
#include <utility>
#include "vpi_user.h"

namespace fin {

class EventRollback {
    std::unique_ptr<s_vpi_vecval[]> vec_;

   public:
    const Signal* signal = nullptr;
    uint64_t time{};
    int bit_idx{};
    s_vpi_value vpi_value{};

    explicit EventRollback(const Signal* signal, uint64_t time, int bit_idx, s_vpi_value* nvalp)
        : signal{signal}, time{time}, bit_idx{bit_idx}, vpi_value{*nvalp} {
        // IEEE 1800-2023 38.15 vpi_get_value()
        const PLI_INT32 numvals = (signal->vpi_width + 31) >> 5;
        vec_ = std::make_unique<s_vpi_vecval[]>(numvals);
        std::ranges::copy_n(nvalp->value.vector, numvals, vec_.get());
        vpi_value.value.vector = vec_.get();
    }

    EventRollback(EventRollback&) = delete;
    EventRollback& operator=(EventRollback&) = delete;
    EventRollback(EventRollback&& other) noexcept
        : vec_{std::move(other.vec_)},
          signal{std::exchange(other.signal, nullptr)},
          time{other.time},
          bit_idx{other.bit_idx},
          vpi_value{std::exchange(other.vpi_value, {})} {}
    EventRollback& operator=(EventRollback&& other) noexcept {
        if (this != &other) {
            vec_ = std::move(other.vec_);
            signal = std::exchange(other.signal, nullptr);
            time = other.time;
            bit_idx = other.bit_idx;
            vpi_value = std::exchange(other.vpi_value, {});
        }
        return *this;
    }

    bool operator<(const EventRollback& other) const { return time < other.time; }

    friend std::ostream& operator<<(std::ostream& os, const EventRollback& ev) {
        return os << "{[" << ev.time << "] " << ev.signal->path << "(" << ev.bit_idx << ")}";
    }

    vpiHandle handle() const { return signal->vpi_handle.handle(); }
    std::string_view sig_path() const { return signal->path; }
};

}  // namespace fin
