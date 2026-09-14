# SPDX-License-Identifier: Apache-2.0

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")

set(SCENARIO_TARGET fi_e2e_coverage_veer)
set(SIMULATION_DIR "${WORK_DIR}")
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_out.csv")

fi_configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/config.json.in"
  DEPENDS "${JSON_NETLIST}"
)

fi_add_fault_campaign(fi_e2e_coverage_veer_campaign
  DEPENDS fi_e2e_veer_basic_netlist "${SIMULATION_DIR}/config.json"
)

fi_add_verilated_sim(fi_e2e_coverage_veer_sim
  VEER_MODE
  COVERAGE
  FAULT_INJECTION
  NO_MAIN NO_BUILD NO_ASSERT
  VERILATOR_EXTRA_ARGS "${VERILATOR_EXTRA_ARGS}"
)

fi_add_sim_run(fi_e2e_coverage_veer_run
  VEER_MODE
  DEPENDS fi_e2e_coverage_veer_sim
)

fi_add_coverage_check(fi_e2e_coverage_veer_coverage_check
  GOLDEN_FILE "${CMAKE_CURRENT_LIST_DIR}/coverage_summary.golden"
  DEPENDS fi_e2e_coverage_veer_run
)

fi_add_ctest_scenario(e2e.coverage.veer ${SCENARIO_TARGET}
  DEPENDS fi_e2e_coverage_veer_coverage_check
)
