# SPDX-License-Identifier: Apache-2.0

macro(define_nested_testbench_vars)
  set(NESTED_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/nested")
  set(NESTED_DESIGN_FILE_LIST "${NESTED_RTL_ROOT}/design.flist")
  set(NESTED_DESIGN_TOP "top")
  set(NESTED_SOURCES
    ${NESTED_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${NESTED_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${NESTED_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${NESTED_DESIGN_TOP}")
  set(SOURCES "${NESTED_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
