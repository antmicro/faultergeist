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

#include "SignalCollector.h"
#include "Utils.h"

#include "sv_vpi_user.h"
#include "vpi_user.h"

#include <utility>

namespace {

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

}  // namespace

namespace fin {

SignalCollector::SignalCollector(std::unordered_map<std::string_view, fin::Signal>& signals)
    : collected_signals(signals) {}

void SignalCollector::collect(vpiHandle scope) {
    vpiHandle scope_iter = vpi_iterate(vpiInternalScope, scope);
    while (fin::ManagedVpiHandle scope_handle = vpi_scan(scope_iter)) {
        collectFromScope(scope_handle.handle());
    }
}

void SignalCollector::insertSignal(fin::Signal signal) {
    // To avoid copying strings throughout the program, the map uses string views into the
    // mapped signals. Extract the new node so its key can point at its mapped value's path.
    auto [it, _] = collected_signals.emplace(std::string_view{}, std::move(signal));
    auto node = collected_signals.extract(it);
    node.key() = node.mapped().path;
    collected_signals.insert(std::move(node));
}

void SignalCollector::collectFromScope(vpiHandle scope_handle) {
    const char* scope_name = vpi_get_str(vpiName, scope_handle);
    fin_printf(indent, const_cast<char*>("scope '%s'\n"), scope_name);

    addIndent();
    vpiHandle signal_iter = vpi_iterate(vpiReg, scope_handle);
    while (ManagedVpiHandle signal_handle = vpi_scan(signal_iter)) {
        collectFromSignal(std::move(signal_handle));
    }
    collect(scope_handle);
    removeIndent();
}

int SignalCollector::collectFromSignal(ManagedVpiHandle signal_handle, const char* name) {
    auto [fn, vpi_type, vpi_width] = VPISignalInfo(signal_handle.handle());
    name = name ? name : fn;
    switch (vpi_type) {
        case vpiRegArray:
            return collectFromArray(std::move(signal_handle), name, vpi_width);
        case vpiReg:
            return collectFromReg(std::move(signal_handle), name, vpi_width);
        case vpiStructVar:
            return collectFromStruct(std::move(signal_handle), name);
        default:
            fin_printf(indent, "Unhandled vpi_type: %d, name: %s\n", vpi_type, name);
            return 0;
    }
}

int SignalCollector::collectFromReg(ManagedVpiHandle handle, const char* name, int vpi_width) {
    fin_printf(indent, "reg: %s, width: %d\n", name, vpi_width);
    const int range_min = findRangeMin(handle.handle(), vpiReg, vpi_width);
    if (vpi_width <= 0) {
        fin_printf(indent, "%%Error: Failed discover signal '%s' of width %d\n", name, vpi_width);
        fin_printf(indent, "Ignoring the signal\n");
        return 0;
    }
    insertSignal(fin::Signal{
        .path = std::string{name},
        .vpi_handle = std::move(handle),
        .vpi_width = vpi_width,
        .range_min = range_min,
        .vpi_type = vpiReg,
    });
    return vpi_width;
}

int SignalCollector::collectFromArray(ManagedVpiHandle handle, const char* name, int vpi_width) {
    fin_printf(indent, "array: %s, size: %d\n", name, vpi_width);
    addIndent();
    const VPIRange array_range = VPIRange(handle.handle());
    int first_index = 0;
    int last_index = vpi_width - 1;
    if (array_range.valid()) {
        std::tie(first_index, last_index) = std::minmax(array_range.left, array_range.right);
    }
    int underlying_elem_size = 0;
    for (int current_index = first_index; current_index <= last_index; ++current_index) {
        std::string elem_name = std::string{name} + '[' + std::to_string(current_index) + ']';
        ManagedVpiHandle elem_handle = vpi_handle_by_index(handle.handle(), current_index);
        assert(elem_handle);
        underlying_elem_size = collectFromSignal(std::move(elem_handle), elem_name.c_str());
    }
    insertSignal(fin::Signal{
        .path = std::string{name},
        .vpi_handle = std::move(handle),
        .vpi_width = vpi_width,
        .range_min = first_index,
        .vpi_type = vpiRegArray,
        .underlying_elem_size = underlying_elem_size
    });
    removeIndent();
    return vpi_width * underlying_elem_size;
}

int SignalCollector::collectFromStruct(ManagedVpiHandle handle, const char* name) {
    fin_printf(indent, "struct: %s, size: %d\n", name, vpi_get(vpiSize, handle.handle()));
    vpiHandle member_iter = vpi_iterate(vpiMember, handle.handle());
    if (!member_iter) {
        fin_printf(indent, "%%Error: Failed to discover members of struct '%s'\n", name);
        return 0;
    }

    addIndent();
    int struct_size = 0;
    std::vector<fin::StructMember> struct_members;
    while (ManagedVpiHandle member_handle = vpi_scan(member_iter)) {
        std::string member_name = vpi_get_str(vpiFullName, member_handle.handle());
        const int member_size = collectFromSignal(std::move(member_handle));
        if (member_size <= 0) {
            continue;
        }
        struct_size += member_size;
        struct_members.push_back(fin::StructMember{
            .name = std::move(member_name),
            .size = member_size,
        });
    }
    removeIndent();
    insertSignal(fin::Signal{
        .path = std::string{name},
        .vpi_handle = std::move(handle),
        .vpi_width = struct_size,
        .vpi_type = vpiStructVar,
        .struct_members = std::move(struct_members),
    });
    return struct_size;
}

int SignalCollector::findRangeMin(vpiHandle signal_handle, int vpi_type, int vpi_width) {
    if (vpi_type != vpiReg) {
        return 0;
    }

    const VPIRange range = VPIRange(signal_handle);
    if (!range.valid()) {
        return 0;
    }

    // If range size doesn't match vpi size, e.g. in case of packed arrays,
    // fallback to min range equal to 0.
    const int range_size =
        std::max(range.left, range.right) - std::min(range.left, range.right) + 1;
    return range_size == vpi_width ? std::min(range.left, range.right) : 0;
}

};  // namespace fin
