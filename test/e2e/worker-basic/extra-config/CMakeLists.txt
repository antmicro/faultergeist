# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_basic_extra_config)
set(WORK_DIR "${WORK_DIR}/extra-config")
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/random_campaign.csv")
set(SIMULATION_DIR "${WORK_DIR}")
set(NETLIST_PATH "${JSON_NETLIST}")
set(EXTRA_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/extra_config.json.in")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/extra_config.json.in" "${EXTRA_CONFIG}")
fi_configure_file("${CMAKE_CURRENT_LIST_DIR}/config.json.in"
  DEPENDS "${NETLIST_PATH}"
)
fi_add_fault_campaign(fi_e2e_worker_basic_extra_config_campaign
  CONFIG_FILE "${CONFIG_FILE}"
  DEPENDS fi_e2e_worker_basic_netlist "${EXTRA_CONFIG}"
)

fi_add_verilated_sim(fi_e2e_worker_basic_extra_config_sim
  FAULT_INJECTION
)

fi_add_sim_run(fi_e2e_worker_basic_extra_config_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_basic_extra_config_sim
)

fi_add_ctest_scenario(e2e.worker.basic.extra-config ${SCENARIO_TARGET}
  DEPENDS fi_e2e_worker_basic_extra_config_run
)
