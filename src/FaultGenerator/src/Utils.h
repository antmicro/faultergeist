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
