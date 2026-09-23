# SPDX-License-Identifier: Apache-2.0

set(TB_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
define_worker_testbench_vars()

set(JSON_NETLIST "${TB_WORK_DIR}/netlist.json")
set(NETLIST_PATH "${JSON_NETLIST}")

set(WORK_DIR "${TB_WORK_DIR}")
fi_add_yosys_json(fi_e2e_coverage_worker_netlist)

foreach(FAULT_TYPE seu set)
  set(WORK_DIR "${TB_WORK_DIR}/${FAULT_TYPE}")
  set(SCENARIO_TARGET fi_e2e_coverage_worker_${FAULT_TYPE})
  set(SIMULATION_DIR "${WORK_DIR}")
  set(FAULT_CAMPAIGN_OUT "${SIMULATION_DIR}/fault_campaign_out.csv")
  fi_add_fault_campaign(fi_e2e_coverage_worker_${FAULT_TYPE}_campaign
    CONFIG_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/config_${FAULT_TYPE}.json.in"
    DEPENDS fi_e2e_coverage_worker_netlist
  )

  fi_add_verilated_sim(fi_e2e_coverage_worker_${FAULT_TYPE}_sim
    COVERAGE
    FAULT_INJECTION
  )

  fi_add_sim_run(fi_e2e_coverage_worker_${FAULT_TYPE}_run
    COVERAGE_TESTING
    DEPENDS fi_e2e_coverage_worker_${FAULT_TYPE}_sim
  )

  fi_add_coverage_check(fi_e2e_coverage_worker_${FAULT_TYPE}_coverage_check
    GOLDEN_FILE "${CMAKE_CURRENT_LIST_DIR}/coverage_summary_${FAULT_TYPE}.golden"
    DEPENDS fi_e2e_coverage_worker_${FAULT_TYPE}_run
  )

  fi_add_ctest_scenario(e2e.coverage.worker.${FAULT_TYPE} ${SCENARIO_TARGET}
    DEPENDS fi_e2e_coverage_worker_${FAULT_TYPE}_coverage_check
  )
endforeach()
