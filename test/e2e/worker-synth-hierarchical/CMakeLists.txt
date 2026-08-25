# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_synth_hierarchical)
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_hierarchical.csv")
set(SIMULATION_DIR "${WORK_DIR}/synth_hierarchical")
define_worker_testbench_vars()

set(JSON_NETLIST "${WORK_DIR}/${DESIGN_TOP}_post_synth.json")

fi_add_yosys_json(fi_e2e_worker_synth_hierarchical_netlist
  SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/synth_hierarchical.tcl"
  DESIGN_FILE_LIST "${WORKER_ROOT}/worker.flist"
)

fi_configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.json.in"
  DEPENDS "${JSON_NETLIST}"
)
fi_add_fault_campaign(fi_e2e_worker_synth_hierarchical_campaign
  DEPENDS fi_e2e_worker_synth_hierarchical_netlist
)

fi_add_verilated_sim(fi_e2e_worker_synth_hierarchical_sim
  FAULT_INJECTION
)

fi_add_sim_run(fi_e2e_worker_synth_hierarchical_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_hierarchical_sim
)

fi_add_ctest_scenario(e2e.worker.synth-hierarchical ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_synth_hierarchical_run
)
