# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_veer_basic_random)
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/random_campaign.csv")
set(SIMULATION_DIR "${WORK_DIR}/random")

fi_configure_file("${CMAKE_CURRENT_LIST_DIR}/config.json.in"
  DEPENDS "${JSON_NETLIST}"
)

fi_add_fault_campaign(fi_e2e_veer_basic_random_campaign
  DEPENDS fi_e2e_veer_basic_netlist
)

fi_add_verilated_sim(fi_e2e_veer_basic_random_sim
  VEER_MODE
  FAULT_INJECTION
  NO_MAIN NO_BUILD
  VERILATOR_EXTRA_ARGS "${VERILATOR_EXTRA_ARGS}"
)

fi_add_sim_run(fi_e2e_veer_basic_random_run
  VEER_MODE
  EXPECT_FAIL
  EXPECT_REGEX_MATCH "Fatal.*el2_veer_wrapper.sv:1003.*Assertion failed"
  DEPENDS fi_e2e_veer_basic_random_sim
)

fi_add_ctest_scenario(e2e.veer.basic.random ${SCENARIO_TARGET}
  DEPENDS fi_e2e_veer_basic_random_run
)
