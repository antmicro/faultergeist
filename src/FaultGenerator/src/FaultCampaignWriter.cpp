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

#include "FaultCampaignWriter.h"

#include "FaultEvent.h"
#include "LogUtils.h"

#include <cassert>
#include <fstream>
#include <ostream>

void FaultCampaignWriter::write(std::ostream& os, const std::vector<FaultEvent>& campaign) const {
    for (const FaultEvent& event : campaign) {
        const auto ev = formatter(event);
        os << ev.time.numerical_value_in(unit::SIM_TIME::unit) << ',';
        os << ev.signal_path << ',';
        os << ev.bit_index << ',';
        os << ev.type << "\n";
    }
}

void FaultCampaignWriter::write(
    const std::string& filepath,
    const std::vector<FaultEvent>& campaign
) const {
    std::ofstream of(filepath);
    SEE_PCHECK(of) << "cannot open '" << filepath << "'. " << "Skipping campaign";
    write(of, campaign);
}
