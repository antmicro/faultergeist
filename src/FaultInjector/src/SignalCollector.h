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

#include "EventParser.h"

#include "Signal.h"

#include <cassert>
#include <cmath>

#include "vpi_user.h"

#pragma once

namespace fin {

class SignalCollector {
   public:
    SignalCollector(std::unordered_map<std::string_view, fin::Signal>& signals);

    void collect(vpiHandle scope);
    void insertSignal(fin::Signal signal);

   private:
    void collectFromScope(vpiHandle scope_handle);
    int collectFromSignal(ManagedVpiHandle signal_handle, const char* name = nullptr);
    int collectFromReg(ManagedVpiHandle handle, const char* name, int vpi_width);
    int collectFromArray(ManagedVpiHandle handle, const char* name, int vpi_width);
    int collectFromStruct(ManagedVpiHandle handle, const char* name);
    static int findRangeMin(vpiHandle signal_handle, int vpi_type, int vpi_width);
    void addIndent() { indent += 1; }
    void removeIndent() { indent -= 1; }

    std::unordered_map<std::string_view, fin::Signal>& collected_signals;
    int indent = 0;
};

};  // namespace fin
