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

#include "FaultEventsSignalFormatter.h"

#include "FaultEvent.h"
#include "LogUtils.h"
#include "Signal.h"
#include "Utils.h"

#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>

#include <cassert>
#include <charconv>

namespace {

// Combines path parts from `hdlname` attribute and removes postfix after `$` character:
// Inputs:
//     prefix_path: `top.proc`
//     hdlname: `worker counter[32:13]$dff`
// Output:
//     `top.proc.worker.counter`.
std::string getSignalPathFromHdlname(std::string_view prefix_path, std::string_view hdlname) {
    if (hdlname.empty()) {
        return "";
    }
    std::vector<std::string_view> path_parts = {prefix_path};
    auto splitter = absl::StrSplit(hdlname, ' ');
    path_parts.insert(path_parts.end(), splitter.begin(), splitter.end());
    std::string joined = absl::StrJoin(path_parts, ".");
    if (auto dollar_pos = findLastNotEscapedDollarSign(joined)) {
        joined = joined.substr(0, *dollar_pos);
    }
    if (auto idxs = findNotEscapedBrackets(joined)) {
        joined = joined.substr(0, idxs->bopen_pos);
    }
    return joined;
}

}  // namespace

FaultEventsSignalFormatter::FaultEventsSignalFormatter(
    std::string_view prefix_path,
    std::span<const Signal> signals
)
    : prefix_path(prefix_path), real_signals_cache(signals.size()), signals(signals) {
    for (std::size_t i = 0; i < signals.size(); i++) {
        insert(i, signals[i]);
    }
}

void FaultEventsSignalFormatter::insertUngrouped(std::size_t id, const Signal& signal) {
    real_signals_cache[id] = SignalData{
        .path = combineSignalPath(signal.path_prefix, signal.cell.getPath()), .bit_idx = 0
    };
}

void FaultEventsSignalFormatter::insert(std::size_t id, const Signal& signal) {
    std::string signal_name{signal.cell.getPath()};
    if (signal.cell.width > 1) {
        VLOG(2) << "[" << signal_name << "] signal.width>1. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }

    auto idxs = findNotEscapedBrackets(signal_name);
    if (!idxs) {
        VLOG(2) << "[" << signal_name << "] is not indexed. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }
    auto [bopen_pos, bclose_pos] = *idxs;

    // When presence of indexing was detected, this call strips the indexing:
    // "top.worker.resp[6]" -> "top.worker.resp"
    std::string_view signal_path{signal_name};
    std::string_view real_signal_name = signal_path.substr(0, bopen_pos);
    std::string_view idx_str = signal_path.substr(bopen_pos + 1, bclose_pos - bopen_pos - 1);
    std::size_t idx;
    const auto [ptr, ec] = std::from_chars(idx_str.data(), idx_str.data() + idx_str.size(), idx);
    if (ec != std::errc{} || ptr != idx_str.data() + idx_str.size()) {
        VLOG(2) << "[" << signal_name << "] is not indexed with a number. Abandoning grouping.";
        insertUngrouped(id, signal);
        return;
    }

    VLOG(2) << "[" << signal_name << "] grouped correctly as {" << real_signal_name << ", " << idx
            << "}";
    real_signals_cache[id] = SignalData{
        .path = combineSignalPath(signal.path_prefix, real_signal_name),
        .hdlname = getSignalPathFromHdlname(prefix_path, signal.cell.hdlname),
        .bit_idx = idx
    };
}

FaultEvent FaultEventsSignalFormatter::operator()(FaultEvent event) const {
    std::size_t id = event.it - signals.begin();
    auto& [path, hdlname, bit_idx] = real_signals_cache[id];
    event.signal_path = hdlname.empty() ? path : hdlname;
    if (event.bit_index == 0) {
        event.bit_index = bit_idx;
    }
    return event;
}
