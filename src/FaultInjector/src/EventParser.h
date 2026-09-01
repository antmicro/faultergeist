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

#include "vpi_user.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace fin {

struct Event;
struct Signal;

class EventParser {
    std::ifstream scenario;
    std::string line_buffer{};

   protected:
    std::unordered_map<std::string_view, Signal> signals;
    double time_multiplier = 1.0;

    EventParser() = default;
    std::optional<Event> parse(std::string_view);
    void insertSignal(Signal);

   public:
    EventParser(const std::filesystem::path&);

    std::optional<Event> parse();
    bool eof() const;
    bool ok() const;

   private:
    void gatherSignals(vpiHandle, int);
    std::nullopt_t printFailedToParseLineError();
};

};  // namespace fin
