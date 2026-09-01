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

#include <charconv>
#include <limits>
#include <optional>

#include <mp-units/compat_macros.h>
#include <mp-units/framework.h>
#include <mp-units/math.h>
#include <mp-units/systems/angular/math.h>
#include <mp-units/systems/si.h>
#include <mp-units/systems/si/math.h>

namespace unit {

using mp_units::quantity;

using mp_units::one;

using mp_units::si::unit_symbols::mg;
using mp_units::si::unit_symbols::rad;

// clang-format off
using mp_units::si::unit_symbols::m;
using mp_units::si::unit_symbols::cm;
using mp_units::si::unit_symbols::mm;
using mp_units::si::unit_symbols::nm;
using mp_units::si::unit_symbols::um;

using mp_units::si::unit_symbols::s;
using mp_units::si::unit_symbols::ms;
using mp_units::si::unit_symbols::us;
using mp_units::si::unit_symbols::ns;
using mp_units::si::unit_symbols::ps;
using mp_units::si::unit_symbols::fs;
// clang-format on

inline constexpr auto MeV = mp_units::si::mega<mp_units::si::electronvolt>;

inline constexpr auto m2 = mp_units::square(m);
inline constexpr auto cm2 = mp_units::square(cm);
inline constexpr auto mm2 = mp_units::square(mm);
inline constexpr auto um2 = mp_units::square(um);
inline constexpr auto nm2 = mp_units::square(nm);

using ONE = quantity<one>;
using RAD = quantity<rad>;
using TIME = quantity<ps>;
using DIST = quantity<um>;
using AREA = quantity<um2>;
using ENERGY = quantity<MeV>;

// NOTE: femtoseconds are hard coded into the injector.
// The unit below should not be changed without adjusting injector
using SIM_TIME = quantity<fs, std::uint64_t>;
const auto MAX_SIM_TIME =
    static_cast<TIME>(std::numeric_limits<SIM_TIME::rep>::max() * SIM_TIME::unit);

using LET = quantity<MeV * cm2 / mg>;
using FLUX = quantity<inverse(s* cm2)>;
using LCS = quantity<cm2>;  // limiting cross section

using mp_units::pow;
using mp_units::sqrt;
using mp_units::si::cos;

template <auto Unit>
SIM_TIME toSimTime(quantity<Unit, double> value) {
    if (value <= value.zero()) {
        return SIM_TIME::zero();
    }
    if (value > MAX_SIM_TIME) {
        value = MAX_SIM_TIME;
    }
    const double converted = value.numerical_value_in(SIM_TIME::unit);
    return static_cast<std::uint64_t>(converted) * SIM_TIME::unit;
}

[[maybe_unused]]
static std::optional<std::tuple<double, std::string_view>> parseTime(std::string_view time_str) {
    double simulation_time;
    const auto [ptr, ec] =
        std::from_chars(time_str.data(), time_str.data() + time_str.size(), simulation_time);
    if (ec != std::errc{} || ptr == time_str.data()) {
        return std::nullopt;
    }

    auto unit = std::string_view(ptr, time_str.data() + time_str.size() - ptr);
    return std::tie(simulation_time, unit);
}

[[maybe_unused]]
static std::optional<SIM_TIME> normalizeSimTime(double value, std::string_view unit) {
    if (unit == "s") {
        return toSimTime(value * s);
    }
    if (unit == "ms") {
        return toSimTime(value * ms);
    }
    if (unit == "us") {
        return toSimTime(value * us);
    }
    if (unit == "ns") {
        return toSimTime(value * ns);
    }
    if (unit == "ps") {
        return toSimTime(value * ps);
    }
    if (unit == "fs") {
        return toSimTime(value * fs);
    }
    return std::nullopt;
}

};  // namespace unit
