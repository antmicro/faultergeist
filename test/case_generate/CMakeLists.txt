# SPDX-License-Identifier: Apache-2.0

macro(define_case_generate_testbench_vars)
  set(CASE_GENERATE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/case_generate")
  set(CASE_GENERATE_DESIGN_FILE_LIST "${CASE_GENERATE_RTL_ROOT}/design.flist")
  set(CASE_GENERATE_DESIGN_TOP "top")
  set(CASE_GENERATE_SOURCES
    ${CASE_GENERATE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${CASE_GENERATE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${CASE_GENERATE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${CASE_GENERATE_DESIGN_TOP}")
  set(SOURCES "${CASE_GENERATE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
