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

struct BendelConfig {
    struct Stream {
        std::string name = "";    // Stream name
        unit::ENERGY energy;      // Proton energy
        unit::FLUX flux_phi;      // Proton fluence rate
        unit::SIM_TIME max_time;  // Duration of the stream

        bool isValid(unit::ENERGY A);
        friend std::ostream& operator<<(std::ostream& os, const Stream& s) {
            os << "{ ";
            std::print(
                os,
                "name={}, energy={}, flux_phi={}, max_time={}",
                s.name,
                s.energy,
                s.flux_phi,
                s.max_time
            );
            return os << " }";
        }
    };
    std::vector<Stream> streams;

    // Fit to the MT4LC4M4B1D28M 3.3 V per-bit cross-sections.
    unit::ENERGY A = 2.4704718 * unit::MeV;
    unit::ENERGY B = 1.7468882 * unit::MeV;

    // Reference cell area adjusting other parameters, compared to source experimental
    // data.
    unit::AREA reference_cell_area = 3.4046173095703125 * unit::um2;
};

class BendelStrategy final : public FaultStrategy {
   public:
    const BendelConfig bendel_config;
    explicit BendelStrategy(const Config&, const BendelConfig&);
    std::vector<FaultEvent> generate(std::span<const Signal> signals) override;
    std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) override;

   private:
    unit::TIME eventTime(const Signal&, const BendelConfig::Stream&, FaultStrategy::RandomGen&);
};
