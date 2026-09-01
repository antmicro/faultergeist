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

#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "Signal.h"

#include <span>
#include <vector>

class RandomStrategy : public FaultStrategy {
   public:
    explicit RandomStrategy(const Config&);
    std::vector<FaultEvent> generate(std::span<const Signal>) override;

    std::shared_ptr<FaultStrategy> copy_with(FaultStrategy::Config) override;
};
