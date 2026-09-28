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

#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "Utils.h"

struct Cell;
struct Signal;
struct Module;
class Liberty;
class PlacementInfo;

class SignalCollector {
    std::string_view top_module;
    std::string_view top_instance;
    std::string combined_prefix_path;
    bool top_is_design_top;

    const Liberty& liberty;
    const PlacementInfo& placement;
    bool collect_wires;
    std::string wire_attribute;
    bool deduplicate_wires;
    std::span<const std::string> clk_names;

   public:
    SignalCollector(
        std::string_view top_module,
        std::string_view top_instance,
        std::string_view prefix_path,
        const Liberty& liberty,
        const PlacementInfo& placement,
        bool collect_wires,
        std::string_view wire_attribute,
        bool deduplicate_wires,
        std::span<const std::string> clk_names
    )
        : top_module(top_module),
          top_instance(top_instance),
          combined_prefix_path(combineSignalPath(prefix_path, top_instance)),
          top_is_design_top(prefix_path.empty()),
          liberty(liberty),
          placement(placement),
          collect_wires(collect_wires),
          wire_attribute(wire_attribute),
          deduplicate_wires(deduplicate_wires),
          clk_names(clk_names) {
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
        Module& module,
        bool is_design_top
    ) const;
};
