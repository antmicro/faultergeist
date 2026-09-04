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

#include "FaultStrategy/FaultStrategy.h"

#include <memory>
#include <string>

struct GlobalOpts final {
    std::string sig_path_prefix;
    std::string top_module;
    std::string top_instance;
    std::string netlist_path;
    std::string fault_campaign_out;
    std::uint64_t campaign_number;
    std::shared_ptr<FaultStrategy> strategy;
    std::vector<std::string> liberty_paths;
    unit::AREA liberty_area_scale;
    std::optional<std::string> vlt_config;
    std::string cell_area_json_path;

    static GlobalOpts parseCmdArgs(int argc, char** argv);
};
