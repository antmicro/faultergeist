# SPDX-License-Identifier: Apache-2.0

macro(define_for_generate_testbench_vars)
  set(FOR_GENERATE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/for_generate")
  set(FOR_GENERATE_DESIGN_FILE_LIST "${FOR_GENERATE_RTL_ROOT}/design.flist")
  set(FOR_GENERATE_DESIGN_TOP "top")
  set(FOR_GENERATE_SOURCES
    ${FOR_GENERATE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${FOR_GENERATE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${FOR_GENERATE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${FOR_GENERATE_DESIGN_TOP}")
  set(SOURCES "${FOR_GENERATE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
