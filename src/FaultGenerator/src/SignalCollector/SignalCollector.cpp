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

#include "SignalCollector.h"

#include "Cell.h"
#include "IsFlipFlopPredicate.h"
#include "Liberty.h"
#include "LogUtils.h"
#include "Module.h"
#include "PlacementInfo.h"
#include "Signal.h"
#include "SlangModuleCollector.h"
#include "Utils.h"
#include "YosysModuleCollector.h"

#include <optional>
#include <sstream>
#include <string_view>

std::vector<Signal> SignalCollector::collectFromFile(const std::filesystem::path& netlist) const {
    std::vector<Module> collected_modules;
    LOG(INFO) << "Collecting modules from '" << netlist << "'";

    if (netlist.extension() == ".json") {
        YosysModuleCollector collector(liberty);
        collected_modules = collector.collectFromFile(netlist);
    } else if (netlist.extension() == ".v" || netlist.extension() == ".sv") {
        SlangModuleCollector collector(liberty);
        collected_modules = collector.collectFromFile(netlist);
    } else {
        SEE_CHECK(false) << "Unknown netlist file extension: " << netlist;
    }
    LOG(INFO) << dumpAllModules(collected_modules);

    return collectFromModules(collected_modules);
}

std::vector<Signal> SignalCollector::collectFromModules(std::vector<Module>& collected_modules
) const {
    int top_module_index = findTopModule(collected_modules);

    std::string starting_path = combineSignalPath(prefix_path, top_instance);

    std::vector<Signal> collected_signals;
    recursivelyCollectSignals(
        collected_signals, starting_path, collected_modules, collected_modules[top_module_index]
    );

    SEE_CHECK(!collected_signals.empty()) << "No signals found, cannot generate faults";
    return collected_signals;
}

int SignalCollector::findTopModule(const std::vector<Module>& modules) const {
    for (unsigned int index = 0; index < modules.size(); ++index) {
        if (modules[index].name == top_module) {
            return index;
        }
    }
    VLOG(3) << "Modules:\n" << dumpAllModules(modules);
    VLOG(3) << "Top module: " << top_module;
    SEE_CHECK(false) << "Top module not found. Cannot generate faults without signals.";
}

std::string SignalCollector::dumpAllModules(const std::vector<Module>& modules) {
    std::stringstream ss;
    for (const auto& mod : modules) {
        ss << mod.dump() << "\n";
    }
    return ss.str();
}

void SignalCollector::recursivelyCollectSignals(
    std::vector<Signal>& collected_signals,
    std::string_view current_path,
    std::vector<Module>& modules,
    Module& module
) const {
    VLOG(2) << "Collecting signals for '" << module.name << "' under prefix: " << current_path;
    for (const Cell& cell : module.cells) {
        std::optional<double> area = liberty.getArea(cell.type);
        SEE_CHECK(area) << "Cell '" << cell.name << "' has no area in liberty";

        auto cell_placement = placement.getCellPlacement(cell.name);
        if (!cell_placement) {
            LOG(WARNING) << "Cell '" << cell.name << "' has no placement info";
        }

        collected_signals.emplace_back(
            std::move(cell),
            std::string{current_path},
            *area,
            std::move(cell_placement),
            SignalType::REGISTER
        );
    }

    for (const auto& [instance_name, module_index] : module.child_modules) {
        std::string next_path = combineSignalPath(current_path, instance_name);
        recursivelyCollectSignals(collected_signals, next_path, modules, modules[module_index]);
    }
}
