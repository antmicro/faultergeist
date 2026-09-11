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
#include "Signal.h"
#include "SignalCollector.h"
#include "Utils.h"

#include <cassert>
#include <charconv>
#include <cmath>
#include <optional>
#include <string>

#include "vpi_user.h"

namespace fin {

// NOTE: femtoseconds are hard coded into the generator.
// The value below should not be changed without adjusting generator
constexpr int FEMTOSECONDS_TIME_PRECISION = -15;

EventParser::EventParser(const std::filesystem::path& scenario_filepath)
    : scenario(scenario_filepath) {
    SignalCollector signal_collector(signals);
    signal_collector.collect(/*handle=*/nullptr);
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

std::pair<const fin::Signal*, int> EventParser::resolveSignal(
    const fin::Signal& signal_to_resolve,
    int index
) {
    if (signal_to_resolve.isArray()) {
        std::string elem_name =
            signal_to_resolve.path + '[' +
            std::to_string(
                signal_to_resolve.range_min + index / signal_to_resolve.underlying_elem_size
            ) +
            ']';
        const Signal* elem = signal(elem_name);
        if (!elem) {
            fin_fatal("Cannot find array element %s!\n", elem_name.c_str());
        }
        return resolveSignal(*elem, index % signal_to_resolve.underlying_elem_size);
    }
    if (signal_to_resolve.isStruct()) {
        if (index < 0 || index >= signal_to_resolve.vpi_width) {
            fin_fatal("Bit index is outside struct %s!\n", signal_to_resolve.path.c_str());
        }
        for (const auto& member : signal_to_resolve.struct_members) {
            if (index < member.size) {
                const Signal* member_signal = signal(member.name);
                if (!member_signal) {
                    fin_fatal("Cannot find struct member %s!\n", member.name.c_str());
                }
                return resolveSignal(*member_signal, index);
            }
            index -= member.size;
        }
        fin_fatal(
            "Bit index did not match any %s struct member!\n", signal_to_resolve.path.c_str()
        );
    }
    return {&signal_to_resolve, index};
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

    auto [resolved_signal, resolved_bit_idx] = resolveSignal(it->second, *bit_idx);

    // Take LSB into account as fault generator doesn't have the data about real signal scope,
    // because signals are broken down into separate bits during synthesis.
    int adjusted_bit_idx =
        resolved_bit_idx - (it->second.isArray() ? 0 : resolved_signal->range_min);

    return Event{
        .signal = resolved_signal,
        .time = scaled_time,
        .bit_idx = adjusted_bit_idx,
        .type = type,
    };
}

std::nullopt_t EventParser::printFailedToParseLineError() {
    fin_printf("%%Error: Failed to parse event line: %s\n", line_buffer.c_str());
    fin_printf("Ignoring the event\n");
    return std::nullopt;
}

const Signal* EventParser::signal(std::string_view name) const {
    if (auto it = signals.find(name); it != signals.end()) {
        return &it->second;
    }
    return nullptr;
}

};  // namespace fin
