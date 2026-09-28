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

#include "Wire.h"

#include <utility>

std::vector<Wire> Wire::removeFlipFlopBits(const std::unordered_set<std::string>& flip_flop_signals
) const {
    if (flip_flop_signals.contains(name)) {
        return {};
    }

    std::vector<Wire> result;
    result.reserve(width);
    for (std::uint32_t position = 0; position < width; ++position) {
        const std::uint32_t index = offset + position;
        std::string bit_name = name + '[' + std::to_string(index) + ']';
        if (!flip_flop_signals.contains(bit_name)) {
            result.emplace_back(Wire{
                .name = std::move(bit_name),
                .hdlname = hdlname,
                .width = 1,
                .offset = index,
                .is_port = is_port,
            });
        }
    }
    return result;
}
