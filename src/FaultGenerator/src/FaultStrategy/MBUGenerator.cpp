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

#include "MBUGenerator.h"

#include "FaultStrategy.h"
#include "LogUtils.h"
#include "PlacementMap.h"
#include "Signal.h"

#include <cmath>
#include <optional>
#include <random>

namespace {

inline constexpr auto distribution_unit = unit::DIST::unit;

std::optional<unit::DIST> radiusToSigma(std::optional<unit::DIST> radius) {
    if (!radius) {
        return std::nullopt;
    }
    const double value = radius->numerical_value_in(distribution_unit);
    SEE_CHECK(std::isfinite(value) && value > 0) << "MBU radius must be finite and positive";
    // Independent normal X/Y offsets give P(distance > r) = exp(-r^2 / (2*sigma^2)).
    // Choose the spread so 95% of sampled points lie inside the requested circle.
    return *radius / std::sqrt(2 * std::log(20.0));
}

Position getCellCenter(const Signal& sig) {
    return {
        .x = sig.cell_placement->x + sig.cell_placement->width / 2,
        .y = sig.cell_placement->y + sig.cell_placement->height / 2,
    };
}

};  //  namespace

MBUGenerator::MBUGenerator(std::span<const Signal> signals, std::optional<unit::DIST> mbu_radius)
    : mbu_sigma(radiusToSigma(mbu_radius)),
      placement_map(mbu_sigma ? PlacementMap(signals) : PlacementMap{}) {}

const Signal* MBUGenerator::generateSecondaryFault(
    FaultStrategy::RandomGen& gen,
    const Signal& primarySignal
) const {
    if (!mbu_sigma || !primarySignal.cell_placement) {
        return nullptr;
    }
    thread_local std::normal_distribution<> dist;
    const std::normal_distribution<>::param_type params{
        0.0, mbu_sigma->numerical_value_in(distribution_unit)
    };
    Position signal_center = getCellCenter(primarySignal);
    Position new_point = {
        .x = signal_center.x + dist(gen.random_generator, params) * distribution_unit,
        .y = signal_center.y + dist(gen.random_generator, params) * distribution_unit,
    };
    return placement_map[new_point];
}
