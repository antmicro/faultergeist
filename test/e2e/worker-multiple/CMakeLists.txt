# SPDX-License-Identifier: Apache-2.0

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(JSON_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.json")
set(SCENARIO_TARGET fi_e2e_worker_multiple)
set(CAMPAIGN_DIR "${WORK_DIR}/fault_campaign_out")
set(CAMPAIGN_STAMP "${WORK_DIR}/fault_campaign_out.stamp")
set(FAULT_CAMPAIGN_OUT "${CAMPAIGN_DIR}")
set(SIMULATION_DIR "${WORK_DIR}/campaign")

define_worker_testbench_vars()

fi_add_yosys_json(fi_e2e_worker_multiple_netlist
)

fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in"
  DEPENDS "${JSON_NETLIST}"
)
fi_add_fault_campaign(fi_e2e_worker_multiple_campaigns
  OUTPUT "${CAMPAIGN_STAMP}"
)

fi_add_multi_campaign_runs(fi_e2e_worker_multiple_campaign_runs
  DEPENDS "${CAMPAIGN_STAMP}"
)

fi_add_ctest_scenario(e2e.worker.multiple ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_multiple_campaign_runs
)
