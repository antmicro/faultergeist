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

#include "Weibull.h"

#include "Constants.h"
#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "LogUtils.h"
#include "Runner.h"
#include "Signal.h"
#include "UnitUtils.h"

#include <cmath>
#include <limits>
#include <vector>

bool WeibullConfig::Stream::isValid(unit::LET let_threshold) {
    const double cos_theta = unit::cos(seu::FLUX_THETA).numerical_value_in(unit::one);
    const auto effective_let = this->let / cos_theta;
    if (effective_let <= let_threshold) {
        LOG(WARNING) << "WeibullStream: { " << *this
                     << " }, will not produce any faults. Please adjust the config.";
        return false;
    }
    return true;
}

WeibullStrategy::WeibullStrategy(const Config& config, const WeibullConfig& weibullConfig)
    : FaultStrategy(config), weibull_config(weibullConfig) {}

unit::TIME WeibullStrategy::eventTime(
    const Signal& signal,
    const WeibullConfig::Stream& stream,
    unit::LCS sigma0,
    FaultStrategy::RandomGen& gen
) {
    thread_local std::exponential_distribution<double> dist;

    const auto cos_theta = unit::cos(seu::FLUX_THETA);
    const auto effective_let = stream.let / cos_theta;

    if (effective_let <= weibull_config.let_threshold) {
        return unit::TIME::max();
    }

    const double pow_arg = ((effective_let - weibull_config.let_threshold) / weibull_config.width)
                               .numerical_value_in(unit::one);

    const auto sigma1 =
        std::pow(pow_arg, weibull_config.shape_parameter.numerical_value_in(unit::one));
    const auto sigma2 = 1.0 - std::exp(-sigma1);
    const unit::quantity<unit::cm2> sigma = sigma0 * sigma2;

    const unit::quantity<inverse(unit::cm2 * unit::s)> flux = stream.flux_phi * cos_theta;
    const unit::quantity<inverse(unit::s)> rate = sigma * flux;
    return dist(gen.random_generator) / rate;
}

std::vector<FaultEvent> WeibullStrategy::generate(std::span<const Signal> signals) {
    LOG(INFO) << "Weibull strategy generating in parallel";

    auto eventTime = [&](const Signal& signal,
                         const WeibullConfig::Stream& stream,
                         FaultStrategy::RandomGen& gen) {
        const unit::ONE limiting_cross_section_factor =
            weibull_config.limiting_cross_section / weibull_config.reference_cell_area;
        const unit::LCS sigma0 =
            signal.area * static_cast<double>(signal.cell.width) * limiting_cross_section_factor;
        return this->eventTime(signal, stream, sigma0, gen);
    };
    auto maxTime = [&](const WeibullConfig::Stream& stream) { return stream.max_time; };
    using WeibullRunner =
        FaultStrategyRunner<WeibullConfig::Stream, decltype(eventTime), decltype(maxTime)>;
    WeibullRunner runner(eventTime, maxTime, config, weibull_config.streams, signals);

    std::vector<FaultEvent> result = runner.generateInParallelByTimeSlice();
    LOG(INFO) << "Weibull strategy generated " << result.size() << " faults";
    return result;
}

std::shared_ptr<FaultStrategy> WeibullStrategy::copy_with(FaultStrategy::Config new_config) {
    return std::make_shared<WeibullStrategy>(new_config, weibull_config);
}
