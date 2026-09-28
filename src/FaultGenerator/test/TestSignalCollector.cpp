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
#include "DesignInfo/Placement.h"
#include "Signal.h"
#include "SignalCollector/IsFlipFlopPredicate.h"
#include "SignalCollector/Module.h"
#include "SignalCollector/SignalCollector.h"
#include "SignalCollector/SlangModuleCollector.h"
#include "SignalCollector/YosysModuleCollector.h"
#include "TestUtils.h"
#include "WireCollectionTestData.h"

#include <gtest/gtest.h>
#include <slang/syntax/SyntaxTree.h>
#include <slang/text/SourceManager.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <vector>

using namespace nlohmann::literals;  // Required for the _json literal

std::string_view normal_top_module = "dff_worker";
std::string_view normal_top_instance = "worker";
std::string_view normal_sig_path_prefix = "top";
Liberty normal_liberty = {{LibertyInfo{
    "test",
    {{"$dff", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}},
     {"$_DFFE_PP_", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}}},
}}};
const PlacementInfo normal_placement{
    std::nullopt,
    {
        CellPlacementInfo{
            "counter$dff",
            "$dff",
            {
                .width = 4 * unit::DIST::unit,
                .height = 3 * unit::DIST::unit,
                .x = 1 * unit::DIST::unit,
                .y = 2 * unit::DIST::unit,
            }
        },
        CellPlacementInfo{
            "resp$dff",
            "$dff",
            {
                .width = 5 * unit::DIST::unit,
                .height = 6 * unit::DIST::unit,
                .x = 8 * unit::DIST::unit,
                .y = 7 * unit::DIST::unit,
            }
        },
    }
};
const bool collect_wires = true;
const std::string_view default_wire_attribute = "hdlname";
const bool deduplicate_wires = true;
const std::vector<std::string> clock_names = {"clk"};

/******************************************************************************/
/* YosysModuleCollector tests                                                 */
/******************************************************************************/

auto normal_json = R"json({
  "creator": "Yosys 0.33 (git sha1 2584903a060)",
  "modules": {
    "dff_worker": {
      "cells": {
        "$add$/path/to/file/worker/dff_worker.v:33$2": {
          "type": "$add",
          "parameters": {
            "A_SIGNED": "00000000000000000000000000000000",
            "A_WIDTH": "00000000000000000000000000100000",
            "B_SIGNED": "00000000000000000000000000000000",
            "B_WIDTH": "00000000000000000000000000100000",
            "Y_WIDTH": "00000000000000000000000000100000"
          }
        },
        "$procmux$4": {
          "type": "$mux",
          "parameters": {
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "$procmux$7": {
          "type": "$mux",
          "parameters": {
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "counter$dff": {
          "type": "$dff",
          "parameters": {
            "CLK_POLARITY": "1",
            "WIDTH": "00000000000000000000000000100000"
          }
        },
        "resp$dff": {
          "type": "$dff",
          "parameters": {
            "CLK_POLARITY": "1",
            "WIDTH": "00000000000000000000000000100000"
          }
        }
      }
    }
  }
})json"_json;

TEST(YosysModuleCollectorTests, EmptyNetlist) {
    auto empty_json = R"json({})json"_json;
    ASSERT_DEATH(
        {
            std::ignore = YosysModuleCollector(
                              normal_liberty,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collect(empty_json);
        },
        "Malformed netlist json"
    );
}

TEST(YosysModuleCollectorTests, ModuleWithNoCells) {
    auto json_without_cells = R"json({
  "creator": "Yosys 0.33 (git sha1 2584903a060)",
  "modules": {
    "dff_worker": {
      "cells": {}
    }
  }
})json"_json;
    auto modules =
        YosysModuleCollector(
            normal_liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(json_without_cells);
    ASSERT_DEATH(
        {
            std::ignore = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collectFromModules(modules);
        },
        "No signals found, cannot generate faults"
    );
}

TEST(YosysModuleCollectorTests, NormalNetlist) {
    const auto& json = normal_json;
    auto modules =
        YosysModuleCollector(
            normal_liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(json);
    const auto& signals = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].cell.getPath(), "counter");
    EXPECT_EQ(signals[0].cell.width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].cell.getPath(), "resp");
    EXPECT_EQ(signals[1].cell.width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
}

TEST(YosysModuleCollectorTests, EmptyLiberty) {
    const auto& json = normal_json;
    ASSERT_DEATH(
        {
            auto modules =
                YosysModuleCollector(
                    {}, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
                )
                    .collect(json);
            const auto& signals = SignalCollector(
                                      normal_top_module,
                                      normal_top_instance,
                                      normal_sig_path_prefix,
                                      {},
                                      normal_placement,
                                      collect_wires,
                                      default_wire_attribute,
                                      deduplicate_wires,
                                      clock_names
            )
                                      .collectFromModules(modules);
        },
        "No signals found, cannot generate faults"
    );
}

/******************************************************************************/
/* SlangModuleCollector tests                                                 */
/******************************************************************************/

auto normal_verilog = R"verilog(
/* Generated by Yosys 0.65 (git sha1 b85cad634, ccache g++ 13.3.0-6ubuntu2~24.04.1 -fPIC -O3) */

(* cells_not_processed =  1  *)
(* src = "dff_worker.v:17.1-38.10" *)
module dff_worker(clk, req_vld, resp_rdy, req, req_rdy, resp_vld, resp, counter);
  (* src = "dff_worker.v:18.11-18.14" *)
  input clk;
  wire clk;
  (* src = "dff_worker.v:19.16-19.23" *)
  input req_vld;
  wire req_vld;
  (* src = "dff_worker.v:20.16-20.24" *)
  input resp_rdy;
  wire resp_rdy;
  (* src = "dff_worker.v:21.23-21.26" *)
  input [31:0] req;
  wire [31:0] req;
  (* src = "dff_worker.v:22.16-22.23" *)
  output req_rdy;
  wire req_rdy;
  (* src = "dff_worker.v:23.16-23.24" *)
  output resp_vld;
  wire resp_vld;
  (* src = "dff_worker.v:24.23-24.27" *)
  output [31:0] resp;
  reg [31:0] resp;
  (* src = "dff_worker.v:25.23-25.30" *)
  output [31:0] counter;
  reg [31:0] counter;
  (* src = "dff_worker.v:30.3-37.6" *)
  wire [31:0] _0_;
  (* src = "dff_worker.v:30.3-37.6" *)
  wire [31:0] _1_;
  (* src = "dff_worker.v:33.18-33.29" *)
  wire [31:0] _2_;
  wire [31:0] _3_;
  wire _4_;
  wire [31:0] _5_;
  wire _6_;
  assign _2_ = counter + (* src = "dff_worker.v:33.18-33.29" *) 32'd1;
  assign _3_ = _4_ ? (* full_case = 32'd1 *) (* src = "dff_worker.v:31.9-31.16|dff_worker.v:31.5-36.8" *) req : 32'd0;
  assign _5_ = _6_ ? (* full_case = 32'd1 *) (* src = "dff_worker.v:31.9-31.16|dff_worker.v:31.5-36.8" *) _2_ : counter;
  (* src = "dff_worker.v:30.3-37.6" *)
  \$_DFFE_PP_ counter$dff (.C(clk), .D(_5_), .E(1'b1), .Q(counter));
  (* src = "dff_worker.v:30.3-37.6" *)
  \$_DFFE_PP_ resp$dff (.C(clk), .D(_3_), .E(1'b1), .Q(resp));
  assign req_rdy = req_vld;
  assign resp_vld = req_vld;
  assign _4_ = req_vld;
  assign _1_ = _3_;
  assign _6_ = req_vld;
  assign _0_ = _5_;
endmodule)verilog";

class SlangModuleCollectorTests : public ::testing::Test {
    std::shared_ptr<slang::syntax::SyntaxTree> tree;
    slang::SourceManager manager;

   protected:
    const std::shared_ptr<slang::syntax::SyntaxTree> normal_tree() { return tree; }
    void SetUp() override {
        const auto& verilog = normal_verilog;
        tree = parseTree(verilog);
    }

    std::shared_ptr<slang::syntax::SyntaxTree> parseTree(
        std::string_view code,
        std::string file_name = ""
    ) {
        if (file_name.empty()) {
            file_name =
                std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) +
                ".sv";
        }
        return slang::syntax::SyntaxTree::fromFileInMemory(code, manager, file_name);
    }
};

TEST_F(SlangModuleCollectorTests, EmptyNetlist) {
    auto tree = parseTree("");
    ASSERT_DEATH(
        {
            std::ignore = SlangModuleCollector(
                              normal_liberty,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collect(tree);
        },
        "No modules found"
    );
}

TEST_F(SlangModuleCollectorTests, ModuleWithNoCells) {
    auto verilog_without_cells = R"verilog(
/* Generated by Yosys 0.65 (git sha1 b85cad634) */

(* cells_not_processed =  1  *)
(* src = "dff_worker.v:17.1-38.10" *)
module dff_worker(clk, req_vld, resp_rdy, req, req_rdy, resp_vld, resp, counter);
endmodule)verilog";
    auto verilog = parseTree(verilog_without_cells);
    auto modules =
        SlangModuleCollector(
            normal_liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(verilog);
    ASSERT_DEATH(
        {
            std::ignore = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collectFromModules(modules);
        },
        "No signals found, cannot generate faults"
    );
}

TEST_F(SlangModuleCollectorTests, NormalNetlist) {
    auto modules =
        SlangModuleCollector(
            normal_liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(normal_tree());
    const auto& signals = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    ASSERT_EQ(signals.size(), 38);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].cell.getPath(), "counter");
    EXPECT_EQ(signals[0].cell.width, 1);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].cell.getPath(), "resp");
    EXPECT_EQ(signals[1].cell.width, 1);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
    for (std::size_t index = 2; index < signals.size(); ++index) {
        EXPECT_EQ(signals[index].type, SignalType::WIRE);
    }
    EXPECT_EQ(signals[2].cell.name, "req_vld[0]");
    EXPECT_EQ(signals[4].cell.name, "req[0]");
    EXPECT_EQ(signals[35].cell.name, "req[31]");
    EXPECT_EQ(signals[36].cell.name, "req_rdy[0]");
    EXPECT_EQ(signals[37].cell.name, "resp_vld[0]");
}

TEST_F(SlangModuleCollectorTests, EmptyLiberty) {
    auto modules = SlangModuleCollector(
                       {}, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
    )
                       .collect(normal_tree());
    const auto signals = SignalCollector(
                             normal_top_module,
                             normal_top_instance,
                             normal_sig_path_prefix,
                             {},
                             normal_placement,
                             collect_wires,
                             default_wire_attribute,
                             deduplicate_wires,
                             clock_names
    )
                             .collectFromModules(modules);

    ASSERT_EQ(signals.size(), 100);
    EXPECT_TRUE(std::ranges::all_of(signals, [](const Signal& signal) {
        return signal.type == SignalType::WIRE;
    }));
}

TEST_F(SlangModuleCollectorTests, RemovesFlipFlopBitsFromNonZeroRange) {
    auto verilog = parseTree(R"verilog(
module top(input wire [6:4] state);
  \$_DFFE_PP_ \state[5]$_DFFE_PP_  ();
endmodule
)verilog");

    const auto modules =
        SlangModuleCollector(
            normal_liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(verilog);
    ASSERT_EQ(modules.size(), 1);
    ASSERT_EQ(modules[0].cells.size(), 1);
    ASSERT_EQ(modules[0].wires.size(), 2);
    EXPECT_EQ(modules[0].wires[0].name, "state[4]");
    EXPECT_EQ(modules[0].wires[0].offset, 4);
    EXPECT_EQ(modules[0].wires[1].name, "state[6]");
    EXPECT_EQ(modules[0].wires[1].offset, 6);
    EXPECT_TRUE(modules[0].wires[0].is_port);
    EXPECT_TRUE(modules[0].wires[1].is_port);
}

/******************************************************************************/
/* SignalCollector tests                                                      */
/******************************************************************************/

const std::vector<Module> normal_modules = {
    {.name = "dff_worker",
     .child_modules = {},
     .cells =
         {{.name = "counter$dff", .type = "$dff", .hdlname = "", .width = 32},
          {.name = "resp$dff", .type = "$dff", .hdlname = "", .width = 32}}}
};

TEST(SignalCollectorTests, EmptyTopModule) {
    auto modules = normal_modules;
    ASSERT_DEATH(
        {
            std::ignore = SignalCollector(
                              "",
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collectFromModules(modules);
        },
        "Top module not found. Cannot generate faults without signals"
    );
}

TEST(SignalCollectorTests, EmptyTopInstance) {
    auto modules = normal_modules;
    ASSERT_DEATH(
        {
            std::ignore = SignalCollector(
                              normal_top_module,
                              "",
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
            )
                              .collectFromModules(modules);
        },
        "Empty top instance! Use --top_instance to specify it's name."
    );
}

TEST(SignalCollectorTests, EmptySigPathPrefix) {
    auto modules = normal_modules;
    const auto& signals = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              "",
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "worker");
    EXPECT_EQ(signals[0].cell.getPath(), "counter");
    EXPECT_EQ(signals[0].cell.width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_EQ(signals[1].path_prefix, "worker");
    EXPECT_EQ(signals[1].cell.getPath(), "resp");
    EXPECT_EQ(signals[1].cell.width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
}

TEST(SignalCollectorTests, NormalNetlistWithPlacement) {
    auto modules = normal_modules;
    const auto& signals = SignalCollector(
                              normal_top_module,
                              normal_top_instance,
                              normal_sig_path_prefix,
                              normal_liberty,
                              normal_placement,
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 2);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].cell.getPath(), "counter");
    EXPECT_EQ(signals[0].cell.width, 32);
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[0].area, 10.0 * unit::AREA::unit);
    ASSERT_TRUE(signals[0].cell_placement);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[0].cell_placement->width, 4 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[0].cell_placement->height, 3 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[0].cell_placement->x, 1 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[0].cell_placement->y, 2 * unit::DIST::unit);

    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].cell.getPath(), "resp");
    EXPECT_EQ(signals[1].cell.width, 32);
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[1].area, 10.0 * unit::AREA::unit);
    ASSERT_TRUE(signals[1].cell_placement);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[1].cell_placement->width, 5 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[1].cell_placement->height, 6 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[1].cell_placement->x, 8 * unit::DIST::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(signals[1].cell_placement->y, 7 * unit::DIST::unit);
}

TEST(SignalCollectorTests, MarksOnlySimulatorTopPortsAsTopLevel) {
    const std::vector<Module> port_modules = {
        {.name = "dut",
         .child_modules = {},
         .cells = {},
         .wires = {{.name = "request[0]", .hdlname = "request", .width = 1, .is_port = true}}}
    };

    auto top_modules = port_modules;
    const auto top_signals = SignalCollector(
                                 "dut",
                                 "top",
                                 "",
                                 {},
                                 {},
                                 collect_wires,
                                 default_wire_attribute,
                                 deduplicate_wires,
                                 clock_names
    )
                                 .collectFromModules(top_modules);
    ASSERT_EQ(top_signals.size(), 1);
    EXPECT_TRUE(top_signals[0].is_top_level_port);

    auto nested_modules = port_modules;
    const auto nested_signals = SignalCollector(
                                    "dut",
                                    "dut",
                                    "top.worker",
                                    {},
                                    {},
                                    collect_wires,
                                    default_wire_attribute,
                                    deduplicate_wires,
                                    clock_names
    )
                                    .collectFromModules(nested_modules);
    ASSERT_EQ(nested_signals.size(), 1);
    EXPECT_FALSE(nested_signals[0].is_top_level_port);
    EXPECT_EQ(nested_signals[0].path_prefix, "top.worker.dut");
}

auto json_with_hdlname = R"json({
  "modules": {
    "worker": {
      "cells": {
        "dff_worker0.counter[0]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 counter$dff"
          }
        },
        "dff_worker0.resp[0]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 resp$dff"
          }
        },
        "dff_worker0.resp[32:13]$_DFFE_PP_": {
          "type": "$_DFFE_PP_",
          "parameters": {
          },
          "attributes": {
            "hdlname": "dff_worker0 resp[32:13]$dff"
          }
        }
      }
    }
  }
}
)json"_json;
const std::string_view json_with_hdlname_top_module = "worker";
const std::string_view json_with_hdlname_top_instance = "worker";
const std::string_view json_with_hdlname_sig_path_prefix = "top";
const Liberty json_with_hdlname_liberty = {{LibertyInfo{
    "test",
    {{"$_DFFE_PP_", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}}},
}}};
const PlacementInfo json_with_hdlname_placement{
    std::nullopt,
    {
        CellPlacementInfo{
            "dff_worker0.counter[0]$_DFFE_PP_",
            "$_DFFE_PP_",
            {
                .width = 4 * unit::DIST::unit,
                .height = 3 * unit::DIST::unit,
                .x = 1 * unit::DIST::unit,
                .y = 2 * unit::DIST::unit,
            }
        },
        CellPlacementInfo{
            "dff_worker0.resp[0]$_DFFE_PP_",
            "$_DFFE_PP_",
            {
                .width = 5 * unit::DIST::unit,
                .height = 6 * unit::DIST::unit,
                .x = 8 * unit::DIST::unit,
                .y = 7 * unit::DIST::unit,
            }
        },
    }
};

TEST(SignalCollectorMiscTests, NetlistWithHdlnameParsing) {
    const auto& json = json_with_hdlname;
    auto modules = YosysModuleCollector(
                       json_with_hdlname_liberty,
                       collect_wires,
                       default_wire_attribute,
                       deduplicate_wires,
                       clock_names
    )
                       .collect(json);
    const auto& signals = SignalCollector(
                              json_with_hdlname_top_module,
                              json_with_hdlname_top_instance,
                              json_with_hdlname_sig_path_prefix,
                              json_with_hdlname_liberty,
                              {},
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    // Check if signals are there
    ASSERT_EQ(signals.size(), 3);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].cell.getPath(), "dff_worker0.counter[0]");
    EXPECT_EQ(signals[0].cell.type, "$_DFFE_PP_");
    EXPECT_EQ(signals[0].cell.width, 1);
    EXPECT_EQ(signals[0].cell.hdlname, "dff_worker0 counter$dff");
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);

    EXPECT_EQ(signals[1].path_prefix, "top.worker");
    EXPECT_EQ(signals[1].cell.getPath(), "dff_worker0.resp[0]");
    EXPECT_EQ(signals[1].cell.type, "$_DFFE_PP_");
    EXPECT_EQ(signals[1].cell.width, 1);
    EXPECT_EQ(signals[1].cell.hdlname, "dff_worker0 resp$dff");
    EXPECT_EQ(signals[1].type, SignalType::REGISTER);

    EXPECT_EQ(signals[2].path_prefix, "top.worker");
    EXPECT_EQ(signals[2].cell.getPath(), "dff_worker0.resp[32:13]");
    EXPECT_EQ(signals[2].cell.type, "$_DFFE_PP_");
    EXPECT_EQ(signals[2].cell.width, 1);
    EXPECT_EQ(signals[2].cell.hdlname, "dff_worker0 resp[32:13]$dff");
    EXPECT_EQ(signals[2].type, SignalType::REGISTER);
}

TEST(SignalCollectorMiscTests, SlangMatchesYosysIgnoringAreaAndWidth) {
#if !FI_E2E_TOOLS_FOUND
    GTEST_SKIP() << "E2E prerequisites unavailable";
#endif

#if !defined(WORKER_JSON_NETLIST) || !defined(WORKER_VERILOG_NETLIST)
#error "WORKER_JSON_NETLIST or WORKER_VERILOG_NETLIST not defined"
#endif

    const Liberty liberty = {{LibertyInfo{
        "test",
        {
            {"AND2x2_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"AOI31xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"DFFHQNx1_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}},
            {"INVx1_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NAND2xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NAND3xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NAND5xp2_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NOR2xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NOR3xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NOR4xp25_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"NOR5xp2_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"OA21x2_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"OAI31xp33_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"OR2x2_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"OR3x1_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"OR5x1_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
            {"XNOR2xp5_ASAP7_75t_R", {.area = 1.0 * unit::AREA::unit, .ff_info = std::nullopt}},
        },
    }}};

    auto collector = SignalCollector(
        normal_top_module,
        normal_top_instance,
        normal_sig_path_prefix,
        liberty,
        normal_placement,
        collect_wires,
        default_wire_attribute,
        deduplicate_wires,
        clock_names
    );
    auto yosys_modules =
        YosysModuleCollector(
            liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collectFromFile(std::string(WORKER_JSON_NETLIST));
    auto yosys_signals = collector.collectFromModules(yosys_modules);
    auto slang_modules =
        SlangModuleCollector(
            liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collectFromFile(std::string(WORKER_VERILOG_NETLIST));
    auto slang_signals = collector.collectFromModules(slang_modules);

    ASSERT_EQ(slang_signals.size(), yosys_signals.size());
    const auto sorter = [](const Signal& sig1, const Signal& sig2) {
        return std::tie(sig1.path_prefix, sig1.cell.name) <
               std::tie(sig2.path_prefix, sig2.cell.name);
    };
    std::multiset<Signal, decltype(sorter)> yosys_set(
        yosys_signals.begin(), yosys_signals.end(), sorter
    );
    std::multiset<Signal, decltype(sorter)> slang_set(
        slang_signals.begin(), slang_signals.end(), sorter
    );

    auto yosys_it = yosys_set.begin(), slang_it = slang_set.begin();

    while (yosys_it != yosys_set.end() && slang_it != slang_set.end()) {
        const auto& sig1 = *yosys_it;
        const auto& sig2 = *slang_it;

        EXPECT_EQ(sig1.path_prefix, sig2.path_prefix);
        EXPECT_EQ(sig1.cell.name, sig2.cell.name);
        EXPECT_EQ(sig1.cell.type, sig2.cell.type);
        EXPECT_EQ(sig1.cell.hdlname, sig2.cell.hdlname);
        EXPECT_EQ(sig1.type, sig2.type);
        ASSERT_EQ(sig1.cell_placement.has_value(), sig2.cell_placement.has_value());
        if (sig1.cell_placement) {
            EXPECT_QUANTITY_DOUBLE_EQ(sig1.cell_placement->width, sig2.cell_placement->width);
            EXPECT_QUANTITY_DOUBLE_EQ(sig1.cell_placement->height, sig2.cell_placement->height);
            EXPECT_QUANTITY_DOUBLE_EQ(sig1.cell_placement->x, sig2.cell_placement->x);
            EXPECT_QUANTITY_DOUBLE_EQ(sig1.cell_placement->y, sig2.cell_placement->y);
        }

        ++yosys_it;
        ++slang_it;
    }
}

// Regression test for a case where FF cells were ignored because liberty cell
// was present in the netlist as a module.
TEST(SignalCollectorMiscTests, NetlistWithLibertyCellIncluded) {
    auto json = R"({
"modules": {
  "my_cell": {
    "cells": {
    }
  },
  "worker": {
    "cells": {
      "dff_worker0.counter[0]$my_cell": {
        "type": "my_cell",
        "parameters": {
        },
        "attributes": {
          "hdlname": "dff_worker0 counter$dff"
        }
      }
    }
  }
}
}
)"_json;
    const std::string_view top_module = "worker";
    const std::string_view top_instance = "worker";
    const std::string_view sig_path_prefix = "top";
    const Liberty liberty = {{LibertyInfo{
        "test",
        {{"my_cell", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}}},
    }}};
    auto modules =
        YosysModuleCollector(
            liberty, collect_wires, default_wire_attribute, deduplicate_wires, clock_names
        )
            .collect(json);
    const auto& signals = SignalCollector(
                              top_module,
                              top_instance,
                              sig_path_prefix,
                              liberty,
                              {},
                              collect_wires,
                              default_wire_attribute,
                              deduplicate_wires,
                              clock_names
    )
                              .collectFromModules(modules);

    // Check if signals is there
    ASSERT_EQ(signals.size(), 1);
    EXPECT_EQ(signals[0].path_prefix, "top.worker");
    EXPECT_EQ(signals[0].cell.getPath(), "dff_worker0.counter[0]");
    EXPECT_EQ(signals[0].cell.type, "my_cell");
    EXPECT_EQ(signals[0].cell.width, 1);
    EXPECT_EQ(signals[0].cell.hdlname, "dff_worker0 counter$dff");
    EXPECT_EQ(signals[0].type, SignalType::REGISTER);
}

enum class NetlistBackend {
    YOSYS,
    SLANG,
};

class WireCollectionTests : public testing::TestWithParam<NetlistBackend> {
   protected:
    const std::string_view top_module = "worker";
    const std::string_view top_instance = "worker";
    const std::string_view sig_path_prefix = "top";
    const Liberty liberty = {{LibertyInfo{
        "test",
        {{"my_cell", {.area = 10.0 * unit::AREA::unit, .ff_info = FlipFlopInfo{}}}},
    }}};

    nlohmann::json json = nlohmann::json::parse(wire_collection_test::json);
    std::string verilog{wire_collection_test::verilog};

    slang::SourceManager source_manager;

    std::vector<Module> collectModules(
        bool collect,
        std::string_view attribute,
        bool deduplicate,
        std::span<const std::string> clocks
    ) {
        if (GetParam() == NetlistBackend::YOSYS) {
            return YosysModuleCollector(liberty, collect, attribute, deduplicate, clocks)
                .collect(json);
        }

        auto tree = slang::syntax::SyntaxTree::fromFileInMemory(
            verilog, source_manager, "wire_collection.sv"
        );
        return SlangModuleCollector(liberty, collect, attribute, deduplicate, clocks).collect(tree);
    }

    std::vector<Signal> collectSignals(
        bool collect,
        std::string_view attribute,
        bool deduplicate,
        std::span<const std::string> clocks
    ) {
        auto modules = collectModules(collect, attribute, deduplicate, clocks);
        return SignalCollector(
                   top_module,
                   top_instance,
                   sig_path_prefix,
                   liberty,
                   /*placement=*/{},
                   collect,
                   attribute,
                   deduplicate,
                   clocks
        )
            .collectFromModules(modules);
    }

    void expectSignals(
        const std::vector<Signal>& signals,
        std::vector<std::pair<std::string, SignalType>> expected
    ) {
        std::vector<std::pair<std::string, SignalType>> actual;
        actual.reserve(signals.size());
        for (const auto& signal : signals) {
            actual.emplace_back(signal.cell.name, signal.type);
        }
        std::ranges::sort(actual);
        std::ranges::sort(expected);
        EXPECT_EQ(actual, expected);
    }
};

// Default wire collection:
// - wire collection is enabled,
// - wires driven by flip-flops are deduplicated,
// - clock is excluded,
// - wires without `hdlname` are omitted.
TEST_P(WireCollectionTests, DefaultWireCollection) {
    const auto signals =
        collectSignals(collect_wires, default_wire_attribute, deduplicate_wires, clock_names);

    expectSignals(
        signals,
        {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER},
         {"out[0]", SignalType::WIRE},
         {"out[1]", SignalType::WIRE},
         {"out[2]", SignalType::WIRE},
         {"out[3]", SignalType::WIRE},
         {"range_wire[8]", SignalType::WIRE},
         {"range_wire[9]", SignalType::WIRE},
         {"range_wire[10]", SignalType::WIRE},
         {"range_wire[11]", SignalType::WIRE},
         {"range_wire[12]", SignalType::WIRE},
         {"range_wire[13]", SignalType::WIRE},
         {"range_wire[14]", SignalType::WIRE},
         {"range_wire[15]", SignalType::WIRE}}
    );
}

TEST_P(WireCollectionTests, NoWireCollection) {
    const auto signals = collectSignals(
        /*collect=*/false, default_wire_attribute, deduplicate_wires, clock_names
    );

    expectSignals(signals, {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER}});
}

TEST_P(WireCollectionTests, CollectsAllWiresWithEmptyAttribute) {
    constexpr std::string_view wire_attribute = "";
    const auto signals =
        collectSignals(collect_wires, wire_attribute, deduplicate_wires, clock_names);

    expectSignals(
        signals,
        {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER},
         {"out[0]", SignalType::WIRE},
         {"out[1]", SignalType::WIRE},
         {"out[2]", SignalType::WIRE},
         {"out[3]", SignalType::WIRE},
         {"range_wire[8]", SignalType::WIRE},
         {"range_wire[9]", SignalType::WIRE},
         {"range_wire[10]", SignalType::WIRE},
         {"range_wire[11]", SignalType::WIRE},
         {"range_wire[12]", SignalType::WIRE},
         {"range_wire[13]", SignalType::WIRE},
         {"range_wire[14]", SignalType::WIRE},
         {"range_wire[15]", SignalType::WIRE},
         {"synth_wire[0]", SignalType::WIRE}}
    );
}

TEST_P(WireCollectionTests, CollectsWiresWithCustomAttribute) {
    constexpr std::string_view wire_attribute = "keep";
    const auto signals =
        collectSignals(collect_wires, wire_attribute, /*deduplicate=*/false, clock_names);

    expectSignals(
        signals,
        {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER},
         {"out[0]", SignalType::WIRE},
         {"out[1]", SignalType::WIRE},
         {"out[2]", SignalType::WIRE},
         {"out[3]", SignalType::WIRE},
         {"synth_wire[0]", SignalType::WIRE}}
    );
}

TEST_P(WireCollectionTests, KeepsFlipFlopWiresWithoutDeduplication) {
    const auto signals =
        collectSignals(collect_wires, default_wire_attribute, /*deduplicate=*/false, clock_names);

    expectSignals(
        signals,
        {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER},
         {"dff_worker0.counter[0]", SignalType::WIRE},
         {"out[0]", SignalType::WIRE},
         {"out[1]", SignalType::WIRE},
         {"out[2]", SignalType::WIRE},
         {"out[3]", SignalType::WIRE},
         {"range_wire[8]", SignalType::WIRE},
         {"range_wire[9]", SignalType::WIRE},
         {"range_wire[10]", SignalType::WIRE},
         {"range_wire[11]", SignalType::WIRE},
         {"range_wire[12]", SignalType::WIRE},
         {"range_wire[13]", SignalType::WIRE},
         {"range_wire[14]", SignalType::WIRE},
         {"range_wire[15]", SignalType::WIRE}}
    );
}

TEST_P(WireCollectionTests, CollectsClockWithoutClockNames) {
    const std::vector<std::string> no_clock_names;
    const auto signals =
        collectSignals(collect_wires, default_wire_attribute, deduplicate_wires, no_clock_names);

    expectSignals(
        signals,
        {{"dff_worker0.counter[0]$my_cell", SignalType::REGISTER},
         {"clk[0]", SignalType::WIRE},
         {"out[0]", SignalType::WIRE},
         {"out[1]", SignalType::WIRE},
         {"out[2]", SignalType::WIRE},
         {"out[3]", SignalType::WIRE},
         {"range_wire[8]", SignalType::WIRE},
         {"range_wire[9]", SignalType::WIRE},
         {"range_wire[10]", SignalType::WIRE},
         {"range_wire[11]", SignalType::WIRE},
         {"range_wire[12]", SignalType::WIRE},
         {"range_wire[13]", SignalType::WIRE},
         {"range_wire[14]", SignalType::WIRE},
         {"range_wire[15]", SignalType::WIRE}}
    );
}

INSTANTIATE_TEST_SUITE_P(
    NetlistBackends,
    WireCollectionTests,
    testing::Values(NetlistBackend::YOSYS, NetlistBackend::SLANG),
    [](const testing::TestParamInfo<NetlistBackend>& info) {
        return info.param == NetlistBackend::YOSYS ? "Yosys" : "Slang";
    }
);
