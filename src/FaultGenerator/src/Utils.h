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

#include <absl/strings/str_cat.h>

#include <cstring>

struct CharSet {
    const char* set;

    CharSet(const char* set) : set(set) {}

    bool contains(char c) { return strchr(set, c); }
};

// Combines two path parts with a dot. Returns only `first` or `second` if the other one is empty.
[[maybe_unused]]
static std::string combineSignalPath(std::string_view first, std::string_view second) {
    if (first.empty()) {
        return std::string{second};
    }
    if (second.empty()) {
        return std::string{first};
    }
    return absl::StrCat(first, ".", second);
}

struct BracketIndices {
    std::size_t bopen_pos;
    std::size_t bclose_pos;
};

// Tries to find indices of unescaped brackets only at the end of the string.
// Returns a `BracketIndices` struct instance, if unescaped brackets are present, containing
// opening bracket char index in `bopen_pos` and closing bracket char index in `bclose_pos`.
// Returns `std::nullopt` otherwise.
[[maybe_unused]]
static std::optional<BracketIndices> findNotEscapedBrackets(std::string_view signal_path) {
    auto bclose_pos = signal_path.size() - 1;
    if (signal_path.size() == 0 || signal_path[bclose_pos] != ']') {
        // path doesn't end with closing brackets, indexing not present
        return std::nullopt;
    }
    if (bclose_pos > 0 && signal_path[bclose_pos - 1] == '\\') {
        // path ends with escaped brackets, indexing not present
        return std::nullopt;
    }
    auto bopen_pos = signal_path.find_last_of('[', bclose_pos);
    if (bopen_pos == signal_path.npos || bopen_pos == 0 || signal_path[bopen_pos - 1] == '\\') {
        // didn't find opening bracket, or encountered an empty signal,
        // or opening brackets are escaped, indexing not present.
        return std::nullopt;
    }
    return BracketIndices{bopen_pos, bclose_pos};
}

// Tries to find an index of unescaped `$` character in provided string. Returns an index of
// unescaped `$` if successful and `std::nullopt` otherwise.
[[maybe_unused]]
static std::optional<std::size_t> findLastNotEscapedDollarSign(std::string_view signal_path) {
    auto dollar_pos = signal_path.find_last_of('$');
    if (dollar_pos != signal_path.npos && dollar_pos >= 1 && signal_path[dollar_pos - 1] != '\\') {
        return dollar_pos;
    }
    return std::nullopt;
}
