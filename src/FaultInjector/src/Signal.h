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

#include "ManagedVpiHandle.h"

#include <string>
#include <vector>

#include "sv_vpi_user.h"

namespace fin {

struct StructMember {
    std::string name;
    int size;
};

struct Signal {
    std::string path;
    ManagedVpiHandle vpi_handle;
    int vpi_width;
    int range_min = 0;
    int vpi_type = vpiReg;
    int underlying_elem_size = 0;  // Size [in bits] of a single array element.
    std::vector<StructMember> struct_members{};

    vpiHandle handle() const { return vpi_handle.handle(); }
    bool isArray() const { return vpi_type == vpiRegArray; }
    bool isStruct() const { return vpi_type == vpiStructVar; }
};

}  // namespace fin
