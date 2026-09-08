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

#include "EventParser.h"

#include "Event.h"
#include "ManagedVpiHandle.h"
#include "Signal.h"
#include "Utils.h"

#include <cassert>
#include <charconv>
#include <cmath>

#include "vpi_user.h"

namespace fin {

// NOTE: femtoseconds are hard coded into the generator.
// The value below should not be changed without adjusting generator
constexpr int FEMTOSECONDS_TIME_PRECISION = -15;

EventParser::EventParser(const std::filesystem::path& scenario_filepath)
    : scenario(scenario_filepath) {
    vpiHandle vhi = vpi_iterate(vpiModule, nullptr);
    gatherSignals(vhi, 0);
    time_multiplier =
        std::pow(10.0, FEMTOSECONDS_TIME_PRECISION - vpi_get(vpiTimePrecision, nullptr));
    if (!scenario) {
        std::error_code ec(errno, std::generic_category());
        fin_printf(
            "%%Error: Failed to open file '%s': %s\n",
            scenario_filepath.c_str(),
            ec.message().c_str()
        );
    }
}

bool EventParser::eof() const {
    return scenario.eof();
}

bool EventParser::ok() const {
    return scenario.operator bool();
}

void EventParser::gatherSignals(vpiHandle it, int indent) {
    while (ManagedVpiHandle hndl = vpi_scan(it)) {
        const char* nm = vpi_get_str(vpiName, hndl.handle());
        for (int i = 0; i < indent; i++) {
            fin_printf(const_cast<char*>("\t"));
        }
        fin_printf(const_cast<char*>("module '%s'\n"), nm);

        vpiHandle vhi = vpi_iterate(vpiReg, hndl.handle());
        assert(vhi);
        while (auto* vh11 = vpi_scan(vhi)) {
            const char* fn = vpi_get_str(vpiFullName, vh11);
            for (int i = 0; i < indent + 1; i++) {
                fin_printf(const_cast<char*>("\t"));
            }
            int vpi_size = vpi_get(vpiSize, vh11);
            fin_printf("reg '%s', width: %d\n", fn, vpi_size);

            if (vpi_size == 0) {
                fin_printf("%%Error: Failed discover signal '%s' of size %d\n", fn, vpi_size);
                fin_printf("Ignoring the signal\n");
                continue;
            }
            insertSignal(Signal{std::string{fn}, ManagedVpiHandle{vh11}, vpi_size});
        }
        vpiHandle scopeIt = vpi_iterate(vpiInternalScope, hndl.handle());
        if (scopeIt) {
            gatherSignals(scopeIt, indent + 1);
        }
    }
}

void EventParser::insertSignal(Signal signal) {
    // To avoid copying strings throughout the probram, where it is not necessary
    // we use map with string_view as keys. So that keys are not dangling pointers,
    // mapped value is the owner of signal_path.
    // This function is to juggle these pointers so that they are pointing correctly.
    auto [it, _] = signals.emplace(std::string_view{}, std::move(signal));
    auto node = signals.extract(it);
    node.key() = node.mapped().path;
    signals.insert(std::move(node));
}

template <typename INT>
std::optional<INT> parseInt(std::string_view str) {
    INT result;
    const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    if (ec != std::errc{} || ptr != str.data() + str.size()) {
        return std::nullopt;
    }
    return result;
}

std::string_view parseCommaSeparated(std::string_view& line) {
    std::size_t end = line.find_first_of(',');
    if (end == line.npos) {
        auto result = line;
        line = "";
        return result;
    }
    auto result = line.substr(0, end);
    line = line.substr(end + 1);
    return result;
}

std::optional<Event> EventParser::parse() {
    if (!std::getline(scenario, line_buffer)) {
        fin_printf("%%Info: No more events to read\n");
        return std::nullopt;
    }
    return parse(line_buffer);
}

std::optional<Event> EventParser::parse(std::string_view line) {
    std::string_view time_str = parseCommaSeparated(line);
    std::string_view sig_path = parseCommaSeparated(line);
    std::string_view bit_idx_str = parseCommaSeparated(line);
    std::string_view type_str = parseCommaSeparated(line);

    if (time_str.empty() || sig_path.empty() || bit_idx_str.empty() || type_str.empty()) {
        return printFailedToParseLineError();
    }

    Event::Type type;
    if (type_str == "set") {
        type = Event::Type::SingleEventTransientUpset;
    } else if (type_str == "seu") {
        type = Event::Type::SingleEventUpset;
    } else {
        return printFailedToParseLineError();
    }

    auto time = parseInt<std::uint64_t>(time_str);
    auto bit_idx = parseInt<int>(bit_idx_str);

    if (!time || !bit_idx) {
        return printFailedToParseLineError();
    }
    std::uint64_t scaled_time = *time * time_multiplier;

    auto it = signals.find(sig_path);
    if (it == signals.end()) {
        fin_printf(
            "%%Error: Unrecognized signal path: %.*s\n", (int)sig_path.size(), sig_path.data()
        );
        fin_printf("Ignoring the event\n");
        return std::nullopt;
    }

    return Event{
        .signal = &it->second,
        .time = scaled_time,
        .bit_idx = *bit_idx,
        .type = type,
    };
}

std::nullopt_t EventParser::printFailedToParseLineError() {
    fin_printf("%%Error: Failed to parse event line: %s\n", line_buffer.c_str());
    fin_printf("Ignoring the event\n");
    return std::nullopt;
}

};  // namespace fin
