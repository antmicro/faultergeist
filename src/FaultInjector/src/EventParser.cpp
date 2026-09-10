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

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <optional>
#include <string>

#include "vpi_user.h"

namespace {

void fin_indent(int indent_size) {
    for (int i = 0; i < indent_size; ++i) {
        fin_printf(const_cast<char*>("\t"));
    }
}

struct VPISignalInfo {
    VPISignalInfo(vpiHandle signal_handle);

    const char* path = nullptr;
    int vpi_type = vpiUndefined;
    int vpi_width = vpiUndefined;
};

VPISignalInfo::VPISignalInfo(vpiHandle signal_handle)
    : path{vpi_get_str(vpiFullName, signal_handle)},
      vpi_type{vpi_get(vpiType, signal_handle)},
      vpi_width{vpi_get(vpiSize, signal_handle)} {}

struct VPIRange {
    VPIRange(vpiHandle signal_handle);

    bool valid() const { return left >= 0 && right >= 0; }

    int left = -1;
    int right = -1;
};

VPIRange::VPIRange(vpiHandle signal_handle) {
    fin::ManagedVpiHandle left_range_handle = vpi_handle(vpiLeftRange, signal_handle);
    fin::ManagedVpiHandle right_range_handle = vpi_handle(vpiRightRange, signal_handle);
    if (!left_range_handle || !right_range_handle) {
        return;
    }

    s_vpi_value range_value{};
    range_value.format = vpiIntVal;
    vpi_get_value(left_range_handle.handle(), &range_value);
    left = range_value.value.integer;
    vpi_get_value(right_range_handle.handle(), &range_value);
    right = range_value.value.integer;
}

int findRangeMin(vpiHandle signal_handle, int vpi_type, int vpi_width) {
    if (vpi_type != vpiReg) {
        return 0;
    }

    const VPIRange range = VPIRange(signal_handle);
    if (!range.valid()) {
        return 0;
    }

    // If range size doesn't match vpi size, e.g. in case of packed arrays, fallback to min range
    // equal to 0.
    const int range_size =
        std::max(range.left, range.right) - std::min(range.left, range.right) + 1;
    return range_size == vpi_width ? std::min(range.left, range.right) : 0;
}

const char* vpiTypeToString(int vpi_type) {
    switch (vpi_type) {
        case vpiReg:
            return "vpiReg";
        case vpiRegArray:
            return "vpiRegArray";
        default:
            return "unknown";
    }
}

}  // namespace

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
    while (ManagedVpiHandle scope_handle = vpi_scan(it)) {
        const char* scope_name = vpi_get_str(vpiName, scope_handle.handle());
        fin_indent(indent);
        fin_printf(const_cast<char*>("scope '%s'\n"), scope_name);

        vpiHandle signal_iter = vpi_iterate(vpiReg, scope_handle.handle());
        assert(signal_iter);
        while (auto* signal_handle = vpi_scan(signal_iter)) {
            auto [fn, vpi_type, vpi_width] = VPISignalInfo(signal_handle);

            if (vpi_width == 0) {
                fin_printf("%%Error: Failed discover signal '%s' of width %d\n", fn, vpi_width);
                fin_printf("Ignoring the signal\n");
                continue;
            }

            const int range_min = findRangeMin(signal_handle, vpi_type, vpi_width);

            switch (vpi_type) {
                case vpiRegArray:
                    gatherArraySignals(signal_handle, fn, vpi_type, vpi_width, indent + 1);
                    break;
                case vpiReg:
                    fin_indent(indent + 1);
                    fin_printf(
                        "reg '%s, width: %d, type: %s'\n", fn, vpi_width, vpiTypeToString(vpi_type)
                    );
                    insertSignal(Signal{
                        .path = std::string{fn},
                        .vpi_handle = ManagedVpiHandle{signal_handle},
                        .vpi_width = vpi_width,
                        .range_min = range_min,
                        .vpi_type = vpi_type
                    });
                    break;
                default:
                    // vpiReg iteration can also return non-bit-vector variables
                    // (for example, the injector instance's string parameter).
                    // They cannot be targets for bit-level fault injection.
                    fin_printf("Ignoring signal '%s' of unsupported vpi type %d\n", fn, vpi_type);
                    vpi_release_handle(signal_handle);
                    break;
            }
        }
        if (vpiHandle scope_it = vpi_iterate(vpiInternalScope, scope_handle.handle())) {
            gatherSignals(scope_it, indent + 1);
        }
    }
}

void EventParser::gatherArraySignals(
    vpiHandle array_handle,
    const char* array_signal_name,
    int array_vpi_type,
    int array_vpi_width,
    int indent_size
) {
    fin_indent(indent_size);
    fin_printf(
        "array '%s, size: %d, type: %s'\n",
        array_signal_name,
        array_vpi_width,
        vpiTypeToString(array_vpi_type)
    );
    const VPIRange array_range = VPIRange(array_handle);
    int first_index = 0;
    int last_index = array_vpi_width;
    if (array_range.valid()) {
        first_index = std::min(array_range.left, array_range.right);
        last_index = std::max(array_range.left, array_range.right);
    }
    int underlying_elem_size = 0;
    for (int current_index = first_index; current_index <= last_index; ++current_index) {
        std::string elem_name =
            std::string{array_signal_name} + '[' + std::to_string(current_index) + ']';
        vpiHandle elem_handle = vpi_handle_by_index(array_handle, current_index);
        assert(elem_handle != nullptr);
        int vpi_elem_type = vpi_get(vpiType, elem_handle);
        int vpi_elem_size = vpi_get(vpiSize, elem_handle);

        if (vpi_elem_size == 0) {
            fin_printf(
                "%%Error: Failed discover signal '%s' of width %d\n",
                elem_name.c_str(),
                vpi_elem_size
            );
            fin_printf("Ignoring the signal\n");
            continue;
        }

        const int range_min = findRangeMin(elem_handle, vpi_elem_type, vpi_elem_size);

        switch (vpi_elem_type) {
            case vpiRegArray: {
                gatherArraySignals(
                    elem_handle, elem_name.c_str(), vpi_elem_type, vpi_elem_size, indent_size + 1
                );
                const Signal* elem_signal = signal(elem_name);
                assert(elem_signal);
                underlying_elem_size = elem_signal->vpi_width * elem_signal->underlying_elem_size;
                break;
            }
            case vpiReg: {
                underlying_elem_size = vpi_elem_size;
                fin_indent(indent_size + 1);

                fin_printf(
                    "reg '%s, width: %d, type: %s'\n",
                    elem_name.c_str(),
                    vpi_elem_size,
                    vpiTypeToString(vpi_elem_type)
                );
                insertSignal(Signal{
                    .path = elem_name,
                    .vpi_handle = ManagedVpiHandle{elem_handle},
                    .vpi_width = vpi_elem_size,
                    .range_min = range_min,
                    .vpi_type = vpi_elem_type
                });
                break;
            }
            default:
                fin_fatal("Unhandled vpi type %d\n", vpi_elem_type);
        }
    }
    insertSignal(Signal{
        .path = std::string{array_signal_name},
        .vpi_handle = ManagedVpiHandle{array_handle},
        .vpi_width = array_vpi_width,
        .range_min = first_index,
        .vpi_type = array_vpi_type,
        .is_unpacked_array = true,
        .underlying_elem_size = underlying_elem_size
    });
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

std::pair<const fin::Signal*, int> EventParser::resolveSignal(
    const fin::Signal& signal_to_resolve,
    int index
) {
    if (signal_to_resolve.is_unpacked_array) {
        std::string elem_name =
            signal_to_resolve.path + '[' +
            std::to_string(
                signal_to_resolve.range_min + index / signal_to_resolve.underlying_elem_size
            ) +
            ']';
        const fin::Signal* elem = signal(elem_name);
        if (!elem) {
            fin_fatal("Cannot find array element %s!\n", elem_name.c_str());
        }
        return resolveSignal(*elem, index % signal_to_resolve.underlying_elem_size);
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
        resolved_bit_idx - (it->second.is_unpacked_array ? 0 : resolved_signal->range_min);

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
