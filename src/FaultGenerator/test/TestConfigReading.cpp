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

#include "FaultStrategy/Bendel.h"
#include "FaultStrategy/Random.h"
#include "FaultStrategy/Weibull.h"
#include "GlobalOpts.h"
#include "TestUtils.h"
#include "UnitUtils.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace nlohmann::literals;  // Required for the _json literal

void from_json(const nlohmann::json&, GlobalOpts&);

TEST(RandomStrategyTests, JsonConfig) {
    auto random_strategy_json =
        R"json({
  "model": {
    "name": "random",
    "params" : {}
  },
  "params": {
    "num_of_events": 101,
    "seed": 13,
    "simulation_time": "102ms",
    "sig_path_prefix": "top",
    "top_module": "dff_worker",
    "top_instance": "worker",
    "netlist_path": "worker.json",
    "fault_campaign_out": "random_file.csv",
    "liberty_area_scale": "26cm2",
    "campaign_number": 26,
    "thread_number": 15
  }
})json"_json;

    GlobalOpts actual;
    ASSERT_NO_THROW({ actual = random_strategy_json.get<GlobalOpts>(); });

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");
    EXPECT_EQ(actual.campaign_number, 26);
    EXPECT_QUANTITY_DOUBLE_EQ(actual.liberty_area_scale, 26 * unit::cm2);

    auto random = std::dynamic_pointer_cast<RandomStrategy>(actual.strategy);
    ASSERT_TRUE(random);
    EXPECT_EQ(random->config.num_of_events, 101);
    EXPECT_EQ(random->config.simulation_time, 102 * unit::ms);
    EXPECT_EQ(random->config.seed, 13);
    EXPECT_EQ(random->config.thread_number, 15u);
}

TEST(WeibullStrategyTests, JsonConfig) {
    auto weibull_strategy_json =
        R"json({
  "model": {
    "name": "weibull",
    "params" : {
      "streams": [{
        "let": 111.0,
        "flux_phi": 123.0,
        "max_time": "321ms"
      }]
    }
  },
  "params": {
    "num_of_events": 103,
    "seed": 56,
    "simulation_time": "104ps",
    "sig_path_prefix": "top",
    "top_module": "dff_worker",
    "top_instance": "worker",
    "netlist_path": "worker.json",
    "fault_campaign_out": "random_file.csv",
    "liberty_area_scale": "17um2",
    "thread_number": 0
  }
})json"_json;

    GlobalOpts actual;
    ASSERT_NO_THROW({ actual = weibull_strategy_json.get<GlobalOpts>(); });

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");
    EXPECT_QUANTITY_DOUBLE_EQ(actual.liberty_area_scale, 17 * unit::um2);

    auto strategy = std::dynamic_pointer_cast<WeibullStrategy>(actual.strategy);
    ASSERT_TRUE(strategy);
    EXPECT_EQ(strategy->config.num_of_events, 103);
    EXPECT_EQ(strategy->config.seed, 56);
    EXPECT_EQ(strategy->config.simulation_time, 104 * unit::ps);
    EXPECT_EQ(strategy->config.thread_number, 1);

    auto wconfig = strategy->weibull_config;
    ASSERT_EQ(wconfig.streams.size(), 1);
    EXPECT_QUANTITY_DOUBLE_EQ(wconfig.streams[0].let, 111.0 * unit::LET::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(wconfig.streams[0].flux_phi, 123.0 * unit::FLUX::unit);
    EXPECT_EQ(wconfig.streams[0].max_time, 321 * unit::ms);
}

TEST(BendelStrategyTests, JsonConfig) {
    auto strategy_json =
        R"json({
  "model": {
    "name": "bendel",
    "params" : {
      "A": 2.0,
      "B": 3.0,
      "streams": [{
        "name": "stream0",
        "energy": 4.0,
        "flux_phi": 5.0,
        "max_time": "6fs"
      }]
    }
  },
  "params": {
    "num_of_events": 103,
    "seed": 56,
    "simulation_time": "104fs",
    "sig_path_prefix": "top",
    "top_module": "dff_worker",
    "top_instance": "worker",
    "netlist_path": "worker.json",
    "fault_campaign_out": "random_file.csv",
    "liberty_area_scale": "15nm2",
    "thread_number": 0

  }
})json"_json;

    GlobalOpts actual;
    ASSERT_NO_THROW({ actual = strategy_json.get<GlobalOpts>(); });

    EXPECT_EQ(actual.sig_path_prefix, "top");
    EXPECT_EQ(actual.top_module, "dff_worker");
    EXPECT_EQ(actual.top_instance, "worker");
    EXPECT_EQ(actual.netlist_path, "worker.json");
    EXPECT_EQ(actual.fault_campaign_out, "random_file.csv");
    EXPECT_QUANTITY_DOUBLE_EQ(actual.liberty_area_scale, 15 * unit::nm2);

    auto strategy = std::dynamic_pointer_cast<BendelStrategy>(actual.strategy);
    ASSERT_TRUE(strategy);
    EXPECT_EQ(strategy->config.num_of_events, 103);
    EXPECT_EQ(strategy->config.seed, 56);
    EXPECT_EQ(strategy->config.simulation_time, 104 * unit::fs);

    auto& config = strategy->bendel_config;
    ASSERT_EQ(config.streams.size(), 1);
    EXPECT_EQ(config.streams[0].name, "stream0");
    EXPECT_QUANTITY_DOUBLE_EQ(config.A, 2.0 * unit::MeV);
    EXPECT_QUANTITY_DOUBLE_EQ(config.B, 3.0 * unit::MeV);
    EXPECT_QUANTITY_DOUBLE_EQ(config.streams[0].energy, 4.0 * unit::MeV);
    EXPECT_QUANTITY_DOUBLE_EQ(config.streams[0].flux_phi, 5.0 * unit::FLUX::unit);
    EXPECT_QUANTITY_DOUBLE_EQ(config.streams[0].max_time, 6.0 * unit::SIM_TIME::unit);
}
