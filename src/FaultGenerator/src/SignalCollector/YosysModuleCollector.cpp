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

#include "YosysModuleCollector.h"

#include "Cell.h"
#include "IsFlipFlopPredicate.h"
#include "LogUtils.h"
#include "Module.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace {

// How many bits parameter.WIDTH takes as a string
constexpr unsigned int SIGNAL_WIDTH_PARAMETER_LENGTH = 32;
unsigned int getSignalWidth(std::string_view width_bits) {
    return std::bitset<SIGNAL_WIDTH_PARAMETER_LENGTH>(std::string(width_bits)).to_ulong();
}

}  // namespace

void YosysModuleCollector::collectCell(Module& mod, Cell cell, const nlohmann::json& json) const {
    if (IsFlipFlop::check(cell, liberty)) {
        VLOG(1) << "Cell '" << cell.name << "' is a flip-flop";
        if (!json.contains("parameters")) {
            LOG(WARNING) << "Cell '" << cell.name
                         << "' has no property 'parameters', Skipping cell.";
            return;
        }

        if (json.contains("attributes")) {
            const auto& attrs = json["attributes"];
            if (attrs.contains("hdlname")) {
                cell.hdlname = attrs["hdlname"].get<std::string>();
            }
        }
        if (cell.hdlname.empty()) {
            VLOG(2) << "Cell '" << cell.name << "' has no 'hldname' attribute. Defaulting "
                    << "to automatically extracted path.";
        }

        const auto& params = json["parameters"];
        if (!params.contains("WIDTH")) {
            VLOG(2) << "Cell '" << cell.name
                    << "' has no property 'parameters.WIDTH', setting value to 1.";
            cell.width = 1;
        } else {
            cell.width = getSignalWidth(params["WIDTH"].get<std::string_view>());
        }

        mod.cells.emplace_back(cell);
        VLOG(3) << "Found signal " << mod.cells.back();
    } else {
        VLOG(1) << "Cell '" << cell.name << "' is not a flip-flop. Skipping";
    }
}

std::vector<Module> YosysModuleCollector::collect(const nlohmann::json& json) const {
    std::unordered_map<std::string, unsigned int> existing_modules;
    std::vector<Module> modules;

    SEE_CHECK(json.contains("modules") && json["modules"].is_object()) << "Malformed netlist json";

    const auto& modules_json = json["modules"];
    // Fill existing_modules
    for (const auto& [name, value] : modules_json.items()) {
        if (value.contains("cells") && value["cells"].is_object()) {
            existing_modules[name] = modules.size();
            modules.emplace_back(name);
        }
    }

    for (const auto& [module_key, module_value] : modules_json.items()) {
        if (!module_value.contains("cells") || !module_value["cells"].is_object()) {
            LOG(WARNING) << "Module '" << module_key << "' contains no cells. Skipping module.";
            continue;
        }

        Module& mod = modules[existing_modules.at(module_key)];
        for (const auto& [cell_key, cell_value] : module_value["cells"].items()) {
            Cell cell{
                .name = cell_key,
                .type = cell_value.value("type", ""),
                .hdlname = "",
                .width = 1,
            };

            if (auto it = existing_modules.find(cell.type); it != existing_modules.end()) {
                VLOG(2) << "Cell's '" << cell.name << "' is child of module '" << it->first << "'";
                mod.child_modules.emplace_back(cell_key, it->second);
            }
            collectCell(mod, std::move(cell), cell_value);
        }
    }
    SEE_CHECK(!modules.empty()) << "No modules found";
    return modules;
}

std::vector<Module> YosysModuleCollector::collectFromFile(const std::filesystem::path& netlist
) const {
    std::ifstream netlist_file(netlist.c_str());
    SEE_PCHECK(netlist_file) << "Cannot access netlist file '" << netlist << "'";

    try {
        nlohmann::json netlist_json = nlohmann::json::parse(netlist_file);
        return collect(netlist_json);
    } catch (...) {
        SEE_CHECK(false) << "Malformed netlist file '" << netlist << "'";
    }
}
