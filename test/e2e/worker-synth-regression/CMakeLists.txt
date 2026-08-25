# SPDX-License-Identifier: Apache-2.0

# Single CTest scenario with two internal subtests:
# Synthesis without techmap and synthesis with techmap

set(SCENARIO_TARGET fi_e2e_worker_synth_regression)
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
define_worker_testbench_vars()

set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_out_no_techmap.csv")
set(JSON_NETLIST "${WORK_DIR}/dff_worker_post_synth_no_techmap.json")
set(SIMULATION_DIR "${WORK_DIR}/no_techmap")
fi_add_yosys_json(fi_e2e_worker_synth_no_techmap_netlist
    NO_TECHMAP
)

fi_configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json.in"
    EXT_LIB_FILES "${RTL_ROOT}/worker.lib"
    DEPENDS "${JSON_NETLIST}"
)
fi_add_fault_campaign(fi_e2e_worker_synth_no_techmap_campaign
  DEPENDS "${JSON_NETLIST}"
)
fi_add_verilated_sim(fi_e2e_worker_synth_no_techmap_sim
  FAULT_INJECTION
)
fi_add_sim_run(fi_e2e_worker_synth_no_techmap_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_no_techmap_sim
)

set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/fault_campaign_with_techmap.csv")
set(JSON_NETLIST "${WORK_DIR}/dff_worker_post_synth.json")
set(SIMULATION_DIR "${WORK_DIR}/with_techmap")
fi_add_yosys_json(fi_e2e_worker_synth_with_techmap_netlist
)

fi_configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json.in"
    DEPENDS "${JSON_NETLIST}"
)
fi_add_fault_campaign(fi_e2e_worker_synth_with_techmap_campaign
  DEPENDS "${JSON_NETLIST}"
)
fi_add_verilated_sim(fi_e2e_worker_synth_with_techmap_sim
  FAULT_INJECTION
)
fi_add_sim_run(fi_e2e_worker_synth_with_techmap_run
  EXPECT_MISMATCH
  DEPENDS fi_e2e_worker_synth_with_techmap_sim
)

fi_add_ctest_scenario(e2e.worker.synth-regression ${SCENARIO_TARGET}
  DEPENDS
    fi_e2e_worker_synth_no_techmap_run
    fi_e2e_worker_synth_with_techmap_run
)
