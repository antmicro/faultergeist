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

#include "UnitUtils.h"

#include <span>
#include <vector>

struct Signal;

struct Position {
    unit::DIST x;
    unit::DIST y;
};

/** Data structure mapping position on the cells on the plane.
 *  In case of overlapping cells, it will return the first cell
 *  inserted.
 */
class PlacementMap {
    std::unordered_map<std::size_t, std::vector<const Signal*>> map;
    Position origin{};
    Position upper{};
    unit::DIST bucket_width = 1 * unit::DIST::unit;
    unit::DIST bucket_height = 1 * unit::DIST::unit;
    std::size_t columns = 1;
    std::size_t rows = 1;

   public:
    PlacementMap() = default;
    PlacementMap(std::span<const Signal>);

    const Signal* operator[](const Position& pos) const;
    const Signal* operator[](unit::DIST, unit::DIST) const;

   private:
    void initializeGrid(std::size_t signal_count);
    void insert(const Signal& signal);
    std::vector<const Signal*> filterSignals(std::span<const Signal>);

    std::size_t getBucketIndex(unit::DIST, unit::DIST) const;
    std::size_t columnAt(unit::DIST x) const;
    std::size_t rowAt(unit::DIST y) const;
    bool isInBounds(unit::DIST, unit::DIST) const;
};
