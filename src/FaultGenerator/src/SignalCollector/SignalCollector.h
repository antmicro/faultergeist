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

#include <absl/log/check.h>
#include <nlohmann/json.hpp>

#include <string_view>

struct Cell;
struct Signal;
struct Module;
class Liberty;
class PlacementInfo;

class SignalCollector {
    std::string_view top_module;
    std::string_view top_instance;
    std::string_view prefix_path;

    const Liberty& liberty;
    const PlacementInfo& placement;

   public:
    SignalCollector(
        std::string_view top_module,
        std::string_view top_instance,
        std::string_view prefix_path,
        const Liberty& liberty,
        const PlacementInfo& placement
    )
        : top_module(top_module),
          top_instance(top_instance),
          prefix_path(prefix_path),
          liberty(liberty),
          placement(placement) {
        CHECK(!top_instance.empty())
            << "Empty top instance! Use --top_instance to specify it's name.";
    }

    std::vector<Signal> collectFromFile(const std::filesystem::path& netlist) const;
    std::vector<Signal> collectFromModules(std::vector<Module>&) const;

   private:
    static std::string dumpAllModules(const std::vector<Module>&);

    int findTopModule(const std::vector<Module>&) const;
    void recursivelyCollectSignals(
        std::vector<Signal>& collected_signals,
        std::string_view current_path,
        std::vector<Module>& modules,
        Module& module
    ) const;
};
