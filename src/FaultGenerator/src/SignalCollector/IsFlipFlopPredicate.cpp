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

#include "IsFlipFlopPredicate.h"

#include "Cell.h"
#include "DesignInfo/Liberty.h"
#include "LogUtils.h"

#include <algorithm>
#include <vector>

class ByKnownFFTypes {
   public:
    bool operator()(const Cell& cell, const Liberty& liberty) const {
        auto result = liberty.isFF(cell.type);
        VLOG(2) << "Trying ByKnownFFTypes predicate, for: " << cell << " with: " << result;
        if (!result && !liberty.contains(cell.type)) {
            LOG(WARNING) << "Cell type '" << cell.type << "' of cell " << cell.name
                         << " is not defined in any liberty file.";
        }
        return result;
    }
};

/*****************************************************************************/

namespace {
const std::vector<IsFlipFlop::PredType> initialized_predicates{
    ByKnownFFTypes(),
};
}  // namespace

bool IsFlipFlop::check(const Cell& cell, const Liberty& liberty) {
    return std::ranges::any_of(initialized_predicates, [&cell, &liberty](const auto& pred) {
        return pred(cell, liberty);
    });
}
