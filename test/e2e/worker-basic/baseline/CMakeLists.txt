# SPDX-License-Identifier: Apache-2.0

set(SCENARIO_TARGET fi_e2e_worker_basic_baseline)
set(SIMULATION_DIR "${WORK_DIR}/baseline")

fi_add_verilated_sim(fi_e2e_worker_basic_baseline_sim
  SOURCES ${WORKER_SOURCES}
  TRACE PUBLIC_FLAT_RW
  TB_TOP "Vtop"
)

fi_add_sim_run(fi_e2e_worker_basic_baseline_run
  TB_TOP "Vtop"
  DEPENDS fi_e2e_worker_basic_baseline_sim
)

fi_add_ctest_scenario(e2e.worker.basic.baseline ${SCENARIO_TARGET}
  DEPENDS fi_e2e_worker_basic_baseline_run
)

