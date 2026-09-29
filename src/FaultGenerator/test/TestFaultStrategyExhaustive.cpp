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

#include "DesignInfo/Liberty.h"
#include "FaultEvent.h"
#include "FaultEventsSignalFormatter.h"
#include "FaultStrategy/Exhaustive.h"
#include "FaultStrategy/MBUGenerator.h"
#include "SignalCollector/Module.h"
#include "SignalCollector/SignalCollector.h"
#include "SignalCollector/SlangModuleCollector.h"
#include "SignalCollector/YosysModuleCollector.h"
#include "Utils.h"
#include "WireCollectionTestData.h"

#include <gtest/gtest.h>
#include <slang/syntax/SyntaxTree.h>
#include <slang/text/SourceManager.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class NetlistBackend {
    YOSYS,
    SLANG,
};

struct FaultEventSnapshot {
    unit::SIM_TIME time;
    std::string signal_path;
    std::uint32_t bit_index;
    FaultEventType type;

    bool operator==(const FaultEventSnapshot&) const = default;
    auto operator<=>(const FaultEventSnapshot&) const = default;
};

class FaultGeneratorTests : public testing::TestWithParam<NetlistBackend> {
   protected:
    static constexpr std::string_view top_module = "worker";
    static constexpr std::string_view top_instance = "top";
    static constexpr std::string_view sig_path_prefix = "";
    static constexpr std::string_view top_port_path_prefix = "TOP";
    static constexpr unit::SIM_TIME simulation_time = 100 * unit::ns;

    const Liberty liberty = {{LibertyInfo{
        "test",
        {{"my_cell", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}}},
    }}};
    slang::SourceManager source_manager;

    std::vector<Module> collectModules(
        bool collect_wires,
        std::string_view wire_attribute,
        bool deduplicate_wires,
        std::span<const std::string> clock_names
    ) {
        if (GetParam() == NetlistBackend::YOSYS) {
            return YosysModuleCollector(
                       liberty, collect_wires, wire_attribute, deduplicate_wires, clock_names
            )
                .collect(nlohmann::json::parse(wire_collection_test::json));
        }

        auto tree = slang::syntax::SyntaxTree::fromFileInMemory(
            wire_collection_test::verilog, source_manager, "fault_generator_test.sv"
        );
        return SlangModuleCollector(
                   liberty, collect_wires, wire_attribute, deduplicate_wires, clock_names
        )
            .collect(tree);
    }

    std::vector<FaultEventSnapshot> generateFaults(
        bool collect_wires,
        std::string_view wire_attribute,
        bool deduplicate_wires,
        std::span<const std::string> clock_names
    ) {
        auto modules =
            collectModules(collect_wires, wire_attribute, deduplicate_wires, clock_names);
        const auto signals = SignalCollector(
                                 top_module,
                                 top_instance,
                                 sig_path_prefix,
                                 liberty,
                                 /*placement=*/{},
                                 collect_wires,
                                 wire_attribute,
                                 deduplicate_wires,
                                 clock_names
        )
                                 .collectFromModules(modules);

        ExhaustiveStrategy strategy(
            {.num_of_events = 0,
             .seed = 42,
             .simulation_time = simulation_time,
             .thread_number = 1,
             .latchup_probability = 0 * unit::PERCENT::unit,
             .transient_probability = 100 * unit::PERCENT::unit},
            {.fault_type = FaultEventType::AUTO}
        );
        const auto events = strategy.generate(MBUGenerator{}, signals);
        const FaultEventsSignalFormatter formatter(
            combineSignalPath(sig_path_prefix, top_instance), signals, top_port_path_prefix
        );

        std::vector<FaultEventSnapshot> result;
        result.reserve(events.size());
        for (const auto& event : events) {
            const auto formatted = formatter(event);
            result.emplace_back(FaultEventSnapshot{
                .time = formatted.time,
                .signal_path = std::string(formatted.signal_path),
                .bit_index = formatted.bit_index,
                .type = formatted.type,
            });
        }
        std::ranges::sort(result);
        return result;
    }
};

TEST_P(FaultGeneratorTests, GeneratesFaultsForAllSignals) {
    const std::vector<std::string> clock_names = {""};
    auto actual = generateFaults(
        /*collect_wires=*/true, /*wire_attribute=*/"", /*deduplicate_wires=*/false, clock_names
    );
    std::vector<FaultEventSnapshot> expected = {
        {0 * unit::ns, "TOP.clk", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
        {0 * unit::ns, "TOP.out", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 1, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 2, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 3, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 8, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 9, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 10, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 11, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 12, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 13, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 14, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 15, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.synth_wire", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {simulation_time, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
    };
    std::ranges::sort(expected);

    EXPECT_EQ(actual, expected);
}

TEST_P(FaultGeneratorTests, GeneratesFaultsWithoutWires) {
    const std::vector<std::string> clock_names = {"clk"};
    auto actual = generateFaults(
        /*collect_wires=*/false,
        /*wire_attribute=*/"hdlname",
        /*deduplicate_wires=*/true,
        clock_names
    );
    std::vector<FaultEventSnapshot> expected = {
        {0 * unit::ns, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
        {simulation_time, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
    };
    std::ranges::sort(expected);

    EXPECT_EQ(actual, expected);
}

TEST_P(FaultGeneratorTests, GeneratesFaultsWithoutClockAndFFDrivenWires) {
    const std::vector<std::string> clock_names = {"clk"};
    auto actual = generateFaults(
        /*collect_wires=*/true, /*wire_attribute=*/"", /*deduplicate_wires=*/true, clock_names
    );
    std::vector<FaultEventSnapshot> expected = {
        {0 * unit::ns, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
        {0 * unit::ns, "TOP.out", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 1, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 2, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "TOP.out", 3, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 8, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 9, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 10, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 11, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 12, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 13, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 14, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.range_wire", 15, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {0 * unit::ns, "top.synth_wire", 0, FaultEventType::SINGLE_EVENT_TRANSIENT},
        {simulation_time, "top.dff_worker0.counter", 0, FaultEventType::SINGLE_EVENT_UPSET},
    };
    std::ranges::sort(expected);

    EXPECT_EQ(actual, expected);
}

INSTANTIATE_TEST_SUITE_P(
    NetlistBackends,
    FaultGeneratorTests,
    testing::Values(NetlistBackend::YOSYS, NetlistBackend::SLANG),
    [](const testing::TestParamInfo<NetlistBackend>& info) {
        return info.param == NetlistBackend::YOSYS ? "Yosys" : "Slang";
    }
);

}  // namespace
