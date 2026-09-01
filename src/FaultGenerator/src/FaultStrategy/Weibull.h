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

#include "FaultStrategy.h"
#include "UnitUtils.h"

#include <span>
#include <vector>

struct Signal;

struct WeibullConfig {
    struct Stream {
        unit::LET let;
        unit::FLUX flux_phi;
        unit::SIM_TIME max_time;

        bool isValid(unit::LET let_threshold);
        friend std::ostream& operator<<(std::ostream& os, const Stream& s) {
            os << "{ ";
            std::print(os, "let={}, flux_phi={}, max_time={}", s.let, s.flux_phi, s.max_time);
            return os << " }";
        }
    };
    std::vector<Stream> streams;

    unit::LET let_threshold = 1.09 * unit::MeV * unit::cm2 / unit::mg;
    unit::LET width = 39.25 * unit::MeV * unit::cm2 / unit::mg;
    unit::ONE shape_parameter = 1.116;

    unit::LCS limiting_cross_section =
        0.28450751 * unit::cm2 /* cm²/device */ / std::pow(2, 22) /* bit width */;

    // Reference cell area adjusting other parameters, compared to source experimental
    // data.
    unit::AREA reference_cell_area = 0.25 * 1e-6 * unit::cm2;
};

class WeibullStrategy : public FaultStrategy {
   public:
    const WeibullConfig weibull_config;

   public:
    explicit WeibullStrategy(const Config&, const WeibullConfig&);
    std::vector<FaultEvent> generate(std::span<const Signal>) override;
    std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) override;

   private:
    unit::TIME eventTime(
        const Signal&,
        const WeibullConfig::Stream&,
        unit::LCS sigma0,
        FaultStrategy::RandomGen&
    );
};
