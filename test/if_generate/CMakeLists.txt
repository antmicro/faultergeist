# SPDX-License-Identifier: Apache-2.0

macro(define_if_generate_testbench_vars)
  set(IF_GENERATE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/if_generate")
  set(IF_GENERATE_DESIGN_FILE_LIST "${IF_GENERATE_RTL_ROOT}/design.flist")
  set(IF_GENERATE_DESIGN_TOP "top")
  set(IF_GENERATE_SOURCES
    ${IF_GENERATE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${IF_GENERATE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${IF_GENERATE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${IF_GENERATE_DESIGN_TOP}")
  set(SOURCES "${IF_GENERATE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
