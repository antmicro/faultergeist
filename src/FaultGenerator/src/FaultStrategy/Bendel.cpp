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

#include "Bendel.h"

#include "Constants.h"
#include "FaultEvent.h"
#include "LogUtils.h"
#include "Runner.h"
#include "Signal.h"
#include "UnitUtils.h"

#include <cmath>
#include <limits>

bool BendelConfig::Stream::isValid(unit::ENERGY A) {
    if (energy <= A) {
        LOG(WARNING) << "BendelStream: " << *this
                     << ", will not produce any faults. Please adjust the config.";
        return false;
    }
    return true;
}

BendelStrategy::BendelStrategy(const Config& config, const BendelConfig& bendelConfig)
    : FaultStrategy(config), bendel_config(bendelConfig) {}

unit::TIME BendelStrategy::eventTime(
    const Signal& signal,
    const BendelConfig::Stream& stream,
    FaultStrategy::RandomGen& gen
) {
    thread_local std::exponential_distribution<double> dist;

    const double energy = stream.energy.numerical_value_in(unit::MeV);
    const double A = bendel_config.A.numerical_value_in(unit::MeV);
    const double B = bendel_config.B.numerical_value_in(unit::MeV);

    if (energy <= A) {
        return unit::TIME::max();
    }

    const double Y = std::sqrt(18.0 / A) * (energy - A);
    const double X = std::pow(B / A, 14.0) * std::pow(1.0 - std::exp(-0.18 * std::sqrt(Y)), 4.0);

    // X is expressed in 10^-12 cm^2/bit by the paper. Normalize that reference-device
    // cross-section by its area per bit, then apply the target Liberty cell area and vector width.
    const unit::LCS reference_cross_section = X * 1e-12 * unit::cm2;
    const unit::ONE cross_section_factor =
        reference_cross_section / bendel_config.reference_cell_area;
    const unit::LCS signal_cross_section =
        signal.area * static_cast<double>(signal.cell.width) * cross_section_factor;

    const auto cos_theta = unit::cos(seu::FLUX_THETA);
    const unit::quantity<unit::one / unit::s> lambda =
        signal_cross_section * stream.flux_phi * cos_theta;
    SEE_INTERNAL_CHECK(lambda > lambda.zero()) << "Assertion in BendelStrategy::eventTime failed";
    return dist(gen.random_generator) / lambda;
}

std::vector<FaultEvent> BendelStrategy::generate(std::span<const Signal> signals) {
    VLOG(1) << "Bendel strategy generating in parallel";

    auto eventTime =
        [&](const Signal& signal, const BendelConfig::Stream& stream, FaultStrategy::RandomGen& gen
        ) { return this->eventTime(signal, stream, gen); };
    auto maxTime = [&](const BendelConfig::Stream& stream) { return stream.max_time; };
    using BendelRunner =
        FaultStrategyRunner<BendelConfig::Stream, decltype(eventTime), decltype(maxTime)>;
    BendelRunner runner(eventTime, maxTime, config, bendel_config.streams, signals);

    std::vector<FaultEvent> result = runner.generateInParallelByTimeSlice();
    VLOG(1) << "Bendel strategy generated " << result.size() << " faults";
    return result;
}

std::shared_ptr<FaultStrategy> BendelStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<BendelStrategy>(new_config, this->bendel_config);
}
