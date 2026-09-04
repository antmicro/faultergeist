# SPDX-License-Identifier: Apache-2.0

set(JSON_VLT_CONFIG "${WORK_DIR}/random-json/random-json.vlt")
set(VERILOG_VLT_CONFIG "${WORK_DIR}/random-verilog/random-verilog.vlt")

block()
  set(SCENARIO_TARGET fi_e2e_worker_basic_random_json)
  set(WORK_DIR "${WORK_DIR}/random-json")
  set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/random_campaign.csv")
  set(SIMULATION_DIR "${WORK_DIR}")
  set(VLT_CONFIG "${JSON_VLT_CONFIG}")
  set(NETLIST_PATH "${JSON_NETLIST}")
  fi_configure_file("${CMAKE_CURRENT_LIST_DIR}/config.json.in"
    DEPENDS "${NETLIST_PATH}"
  )
  fi_add_fault_campaign(fi_e2e_worker_basic_random_json_campaign
    DEPENDS fi_e2e_worker_basic_netlist
  )

  fi_add_verilated_sim(fi_e2e_worker_basic_random_json_sim
    FAULT_INJECTION
  )

  fi_add_sim_run(fi_e2e_worker_basic_random_json_run
    EXPECT_MISMATCH
    DEPENDS fi_e2e_worker_basic_random_json_sim
  )

  fi_add_ctest_scenario(e2e.worker.basic.random.json ${SCENARIO_TARGET}
    DEPENDS fi_e2e_worker_basic_random_json_run
  )
endblock()

block()
  set(SCENARIO_TARGET fi_e2e_worker_basic_random_verilog)
  set(WORK_DIR "${WORK_DIR}/random-verilog")
  set(FAULT_CAMPAIGN_OUT "${WORK_DIR}/random_campaign.csv")
  set(SIMULATION_DIR "${WORK_DIR}")
  set(VLT_CONFIG "${VERILOG_VLT_CONFIG}")
  set(NETLIST_PATH "${VERILOG_NETLIST}")
  fi_configure_file("${CMAKE_CURRENT_LIST_DIR}/config.json.in"
    DEPENDS "${NETLIST_PATH}"
  )
  fi_add_fault_campaign(fi_e2e_worker_basic_random_verilog_campaign
    DEPENDS fi_e2e_worker_basic_netlist
  )

  fi_add_verilated_sim(fi_e2e_worker_basic_random_verilog_sim
    FAULT_INJECTION
  )

  fi_add_sim_run(fi_e2e_worker_basic_random_verilog_run
    EXPECT_MISMATCH
    DEPENDS fi_e2e_worker_basic_random_verilog_sim
  )

  fi_add_ctest_scenario(e2e.worker.basic.random.verilog ${SCENARIO_TARGET}
    DEPENDS fi_e2e_worker_basic_random_verilog_run
  )
endblock()

fi_add_file_comparison(fi_e2e_worker_basic_random_vlt_config_compare
  "${JSON_VLT_CONFIG}"
  "${VERILOG_VLT_CONFIG}"
  DEPENDS
    fi_e2e_worker_basic_random_json_campaign
    fi_e2e_worker_basic_random_verilog_campaign
)

fi_add_ctest_scenario(e2e.worker.basic.random.vlt-config
  fi_e2e_worker_basic_random_vlt_config
  DEPENDS fi_e2e_worker_basic_random_vlt_config_compare
)
