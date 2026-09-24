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

#include "PlacementMap.h"

#include "LogUtils.h"
#include "Signal.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
bool finite(unit::DIST value) {
    return std::isfinite(value.numerical_value_in(unit::DIST::unit));
}

std::size_t coordinate(unit::DIST offset, unit::DIST size, std::size_t count) {
    const double index = std::floor((offset / size).numerical_value_in(unit::one));
    return static_cast<std::size_t>(std::clamp(index, 0.0, static_cast<double>(count - 1)));
}

bool isValid(const Placement& p) {
    return finite(p.x) && finite(p.y) && finite(p.width) && finite(p.height) &&
           finite(p.x + p.width) && finite(p.y + p.height) && p.width > unit::DIST::zero() &&
           p.height > unit::DIST::zero();
}

}  // namespace

PlacementMap::PlacementMap(std::span<const Signal> signals) {
    std::vector<const Signal*> placed = filterSignals(signals);
    if (placed.empty()) {
        return;
    }

    initializeGrid(placed.size());
    map.reserve(signals.size());
    for (const auto* signal : placed) {
        insert(*signal);
    }
}

std::vector<const Signal*> PlacementMap::filterSignals(std::span<const Signal> signals) {
    std::vector<const Signal*> result;
    for (const auto& signal : signals) {
        if (!signal.cell_placement) {
            LOG(WARNING) << "Signal has no placement: " << signal.cell.name;
            continue;
        }
        const Placement& p = *signal.cell_placement;
        if (!isValid(p)) {
            LOG(WARNING) << "Signal has invalid placement: " << signal.cell.name;
            continue;
        }
        if (result.empty()) {
            origin = {p.x, p.y};
            upper = {p.x + p.width, p.y + p.height};
        } else {
            origin = {std::min(origin.x, p.x), std::min(origin.y, p.y)};
            upper = {std::max(upper.x, p.x + p.width), std::max(upper.y, p.y + p.height)};
        }
        result.push_back(&signal);
    }

    return result;
}

void PlacementMap::initializeGrid(std::size_t n) {
    const auto width = upper.x - origin.x;
    const auto height = upper.y - origin.y;
    if (n > std::numeric_limits<std::size_t>::max() / 2) {
        LOG(WARNING) << "Placement grid count would overflow; using a single bucket";
        columns = rows = 1;
        bucket_width = width;
        bucket_height = height;
        return;
    }
    const double ratio = (width / height).numerical_value_in(unit::one);
    const double estimate = std::ceil(std::sqrt(n * ratio));
    columns =
        estimate >= static_cast<double>(n) ? n : static_cast<std::size_t>(std::max(1.0, estimate));
    rows = n / columns + (n % columns != 0);
    bucket_width = width / columns;
    bucket_height = height / rows;
    SEE_CHECK(bucket_width > unit::DIST::zero() && bucket_height > unit::DIST::zero())
        << "Internal error: placement bucket dimensions underflow";
}

void PlacementMap::insert(const Signal& signal) {
    const auto& p = *signal.cell_placement;
    const auto first_column = columnAt(p.x);
    const auto first_row = rowAt(p.y);
    const auto last_column = columnAt(p.x + p.width);
    const auto last_row = rowAt(p.y + p.height);
    for (auto r = first_row; r <= last_row; ++r) {
        for (auto c = first_column; c <= last_column; ++c) {
            map[r * columns + c].push_back(&signal);
        }
    }
}

const Signal* PlacementMap::operator[](unit::DIST x, unit::DIST y) const {
    if (map.empty() || !isInBounds(x, y)) {
        return nullptr;
    }
    auto bucket = map.find(getBucketIndex(x, y));
    if (bucket == map.end()) {
        return nullptr;
    }
    const auto found = std::ranges::find_if(bucket->second, [=](const Signal* signal) {
        return signal->cell_placement->containsPoint(x, y);
    });
    return found == bucket->second.end() ? nullptr : *found;
}

const Signal* PlacementMap::operator[](const Position& pos) const {
    return (*this)[pos.x, pos.y];
}

std::size_t PlacementMap::columnAt(unit::DIST x) const {
    return coordinate(x - origin.x, bucket_width, columns);
}

std::size_t PlacementMap::rowAt(unit::DIST y) const {
    return coordinate(y - origin.y, bucket_height, rows);
}

std::size_t PlacementMap::getBucketIndex(unit::DIST x, unit::DIST y) const {
    return rowAt(y) * columns + columnAt(x);
}

bool PlacementMap::isInBounds(unit::DIST x, unit::DIST y) const {
    return finite(x) && finite(y) && x > origin.x && y > origin.y && x < upper.x && y < upper.y;
}
