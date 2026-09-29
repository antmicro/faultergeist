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

#include "FaultEvent.h"
#include "RandomGen.h"
#include "Signal.h"
#include "UnitUtils.h"

#include <gtest/gtest.h>

namespace {

FaultEventConfig cfg(double latchup_pct, RandomGen& gen) {
    return {
        .latchup_probability = latchup_pct * unit::PERCENT::unit,
        .transient_probability = (100.0 - latchup_pct) * unit::PERCENT::unit,
        .gen = gen,
    };
}

}  // namespace

TEST(SingleEventLatchupTest, RegisterAlwaysSeuIgnoresLatchupProbability) {
    RandomGen gen{42};
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(
            faultEventType(SignalType::REGISTER, cfg(100.0, gen)),
            FaultEventType::SINGLE_EVENT_UPSET
        );
    }
}

TEST(SingleEventLatchupTest, WireAtHundredPercentIsAlwaysLatchup) {
    RandomGen gen{42};
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(
            faultEventType(SignalType::WIRE, cfg(500.0, gen)), FaultEventType::SINGLE_EVENT_LATCHUP
        );
    }
}

TEST(SingleEventLatchupTest, WireAtZeroPercentIsAlwaysTransient) {
    RandomGen gen{42};
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(
            faultEventType(SignalType::WIRE, cfg(0.0, gen)), FaultEventType::SINGLE_EVENT_TRANSIENT
        );
    }
}

TEST(SingleEventLatchupTest, WireAtFiftyPercentIsApproximatelyHalf) {
    RandomGen gen{42};
    constexpr int samples = 10000;
    int latchup = 0;
    int transient = 0;
    for (int i = 0; i < samples; ++i) {
        auto type = faultEventType(SignalType::WIRE, cfg(50.0, gen));
        if (type == FaultEventType::SINGLE_EVENT_LATCHUP) {
            ++latchup;
        } else {
            ASSERT_EQ(type, FaultEventType::SINGLE_EVENT_TRANSIENT);
            ++transient;
        }
    }
    EXPECT_NEAR(static_cast<double>(latchup) / samples, 0.5, 0.02)
        << "latchup=" << latchup << " transient=" << transient;
}
