# SPDX-License-Identifier: Apache-2.0

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
define_bit_range_testbench_vars()

set(JSON_NETLIST "${WORK_DIR}/netlist.json")
set(NETLIST_PATH "${JSON_NETLIST}")

fi_add_yosys_json(fi_e2e_coverage_bit_range_netlist)

set(SCENARIO_TARGET fi_e2e_coverage_bit_range_json)
set(SIMULATION_DIR "${WORK_DIR}")
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_out.csv")
fi_configure_file(
  "${COVERAGE_CONFIG_TEMPLATE}"
  DEPENDS "${JSON_NETLIST}"
)

fi_add_fault_campaign(fi_e2e_coverage_bit_range_json_campaign
  DEPENDS fi_e2e_coverage_bit_range_netlist
)

fi_add_verilated_sim(fi_e2e_coverage_bit_range_json_sim
  COVERAGE
  FAULT_INJECTION
  NO_MAIN
  TRACE
)

fi_add_sim_run(fi_e2e_coverage_bit_range_json_run
  DEPENDS fi_e2e_coverage_bit_range_json_sim
)

fi_add_coverage_check(fi_e2e_coverage_bit_range_json_coverage_check
  GOLDEN_FILE "${CMAKE_CURRENT_LIST_DIR}/coverage_summary.golden"
  DEPENDS fi_e2e_coverage_bit_range_json_run
)

fi_add_ctest_scenario(e2e.coverage.bit_range.json ${SCENARIO_TARGET}
  DEPENDS fi_e2e_coverage_bit_range_json_coverage_check
)
