# SPDX-License-Identifier: Apache-2.0

set(SIM_MAIN_FILE "${PROJECT_SOURCE_DIR}/test/SimMain.cpp")
set(COVERAGE_CONFIG_SEU_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/config_seu.json.in")
set(COVERAGE_CONFIG_SET_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/config_set.json.in")
set(VERILATOR_EXTRA_ARGS -timescale-override 1fs/1fs)

function(fi_add_coverage_e2e_test NAME FAULT_TYPE)
  string(TOUPPER "${FAULT_TYPE}" FAULT_TYPE_UPPER)
  set(TB_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
  set(JSON_NETLIST "${TB_WORK_DIR}/netlist.json")
  set(NETLIST_PATH "${JSON_NETLIST}")

  if(NOT TARGET fi_e2e_coverage_${NAME}_netlist)
    set(WORK_DIR "${TB_WORK_DIR}")
    fi_add_yosys_json(fi_e2e_coverage_${NAME}_netlist)
  endif()

  set(WORK_DIR "${TB_WORK_DIR}/${FAULT_TYPE}")
  set(SIMULATION_DIR "${WORK_DIR}")
  set(FAULT_CAMPAIGN_OUT "${SIMULATION_DIR}/fault_campaign_out.csv")

  fi_add_fault_campaign(fi_e2e_coverage_${NAME}_${FAULT_TYPE}_campaign
    CONFIG_TEMPLATE "${COVERAGE_CONFIG_${FAULT_TYPE_UPPER}_TEMPLATE}"
    DEPENDS fi_e2e_coverage_${NAME}_netlist
  )

  fi_add_verilated_sim(fi_e2e_coverage_${NAME}_${FAULT_TYPE}_sim
    COVERAGE
    FAULT_INJECTION
    NO_MAIN
    TRACE
  )

  fi_add_sim_run(fi_e2e_coverage_${NAME}_${FAULT_TYPE}_run
    COVERAGE_TESTING
    DEPENDS fi_e2e_coverage_${NAME}_${FAULT_TYPE}_sim
  )

  fi_add_coverage_check(fi_e2e_coverage_${NAME}_${FAULT_TYPE}_coverage_check
    GOLDEN_FILE "${CMAKE_CURRENT_LIST_DIR}/coverage_summary_${FAULT_TYPE}.golden"
    DEPENDS fi_e2e_coverage_${NAME}_${FAULT_TYPE}_run
  )

  fi_add_ctest_scenario(e2e.coverage.${NAME}.${FAULT_TYPE}
    fi_e2e_coverage_${NAME}_${FAULT_TYPE}
    DEPENDS fi_e2e_coverage_${NAME}_${FAULT_TYPE}_coverage_check
  )
endfunction()

add_subdirectory(bit_range)
add_subdirectory(case_generate)
add_subdirectory(for_generate)
add_subdirectory(generate_instances)
add_subdirectory(if_generate)
add_subdirectory(instance_array)
add_subdirectory(multi_instance)
add_subdirectory(multi_scope_signal)
add_subdirectory(named_proc_block)
add_subdirectory(nested)
add_subdirectory(nested_generate)
add_subdirectory(packed_unpacked)
add_subdirectory(range)
add_subdirectory(signal_widths)
add_subdirectory(structs)
add_subdirectory(worker)
