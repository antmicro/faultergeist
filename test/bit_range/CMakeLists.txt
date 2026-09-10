# SPDX-License-Identifier: Apache-2.0

macro(define_bit_range_testbench_vars)
  set(BIT_RANGE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/bit_range")
  set(BIT_RANGE_DESIGN_FILE_LIST "${BIT_RANGE_RTL_ROOT}/design.flist")
  set(BIT_RANGE_DESIGN_TOP "top")
  set(BIT_RANGE_SOURCES
    ${BIT_RANGE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${BIT_RANGE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${BIT_RANGE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${BIT_RANGE_DESIGN_TOP}")
  set(SOURCES "${BIT_RANGE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
