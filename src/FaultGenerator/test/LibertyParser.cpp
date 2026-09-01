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

#include "LibertyParser.h"
#include "LogUtils.h"
#include "UnitUtils.h"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>

ABSL_FLAG(std::string, input_path, "", "[REQUIRED] input file");

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);

    CHECK(!absl::GetFlag(FLAGS_input_path).empty()) << "Missing \"--input_path\" flag";

    auto parse_result = LibertyParser(1 * unit::um2).parse(absl::GetFlag(FLAGS_input_path));
    CHECK(parse_result) << "Failed to parse";

    std::cout << "library\n";
    for (const auto& [cell_name, cell_info] : parse_result->cells) {
        std::cout << "cell(\"%s\") " << cell_info << "\n";
    }
}
