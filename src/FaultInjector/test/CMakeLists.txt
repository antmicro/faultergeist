# SPDX-License-Identifier: Apache-2.0

add_subdirectory(unit)

set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
define_worker_testbench_vars()
set(SOURCES ${WORKER_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/TestWorker.cpp)

set(SIMULATION_DIR "${WORK_DIR}/baseline")
fi_add_verilated_sim(fi_integration_injection_baseline_sim
  TRACE
  NO_MAIN
)
fi_add_sim_run(fi_integration_injection_baseline_run
  DEPENDS fi_integration_injection_baseline_sim
  EXPECT_VCD_GOLDENFILE "${CMAKE_CURRENT_LIST_DIR}/baseline_dump.vcd.out"
  WORK_DIR "${SIMULATION_DIR}"
)
add_test(
  NAME integration.injection.vcdFaultGoldenfile.baseline
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target fi_integration_injection_baseline_run --config $<CONFIG>
)


set(FAULT_CAMPAIGN_OUT "${CMAKE_CURRENT_LIST_DIR}/fault_injector_set_test.csv.in")
set(SIMULATION_DIR "${WORK_DIR}/set_test")
fi_add_verilated_sim(fi_integration_injection_set_sim
  TRACE FAULT_INJECTION
  NO_MAIN
)
fi_add_sim_run(fi_integration_injection_set_run
  DEPENDS fi_integration_injection_set_sim
  EXPECT_MISMATCH
  EXPECT_VCD_GOLDENFILE "${CMAKE_CURRENT_LIST_DIR}/set_dump.vcd.out"
  WORK_DIR "${SIMULATION_DIR}"
)
add_test(
  NAME integration.injection.vcdFaultGoldenfile.set
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target fi_integration_injection_set_run --config $<CONFIG>
)


set(FAULT_CAMPAIGN_OUT "${CMAKE_CURRENT_LIST_DIR}/fault_injector_seu_test.csv.in")
set(SIMULATION_DIR "${WORK_DIR}/seu_test")
fi_add_verilated_sim(fi_integration_injection_seu_sim
  TRACE FAULT_INJECTION
  NO_MAIN
  SOURCES ${SOURCES}
)
fi_add_sim_run(fi_integration_injection_seu_run
  DEPENDS fi_integration_injection_seu_sim
  EXPECT_MISMATCH
  EXPECT_VCD_GOLDENFILE "${CMAKE_CURRENT_LIST_DIR}/seu_dump.vcd.out"
  WORK_DIR "${SIMULATION_DIR}"
)
add_test(
  NAME integration.injection.vcdFaultGoldenfile.seu
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target fi_integration_injection_seu_run --config $<CONFIG>
)


set(SIMULATION_DIR "${WORK_DIR}/missing_campaign_file")
fi_add_verilated_sim(fi_integration_missing_campaign_file_sim
  FAULT_INJECTION CAMPAIGN_FILE "non_existent.csv"
  SOURCES ${WORKER_SOURCES}
)
fi_add_sim_run(fi_integration_missing_campaign_file_run
  EXPECT_REGEX "Failed to open file 'non_existent.csv': No such file or directory"
  DEPENDS fi_integration_missing_campaign_file_sim
  WORK_DIR "${SIMULATION_DIR}"
)
add_test(
  NAME integration.injection.missing_campaign_file
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target fi_integration_missing_campaign_file_run --config $<CONFIG>
)

