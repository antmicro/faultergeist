# SPDX-License-Identifier: Apache-2.0

define_structs_testbench_vars()
fi_add_coverage_e2e_test(structs seu)

# Structs `set` skipped due to Verilator bug (ant #104826)
add_test(
  NAME e2e.coverage.structs.set
  COMMAND bash -c "exit ${FI_E2E_SKIP_CODE}"
)
set_tests_properties(e2e.coverage.structs.set
 PROPERTIES
    LABELS "e2e;yosys;verilator"
    SKIP_RETURN_CODE ${FI_E2E_SKIP_CODE}
    TIMEOUT 3600
)
