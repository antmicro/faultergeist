# SPDX-License-Identifier: Apache-2.0

macro(define_range_testbench_vars)
  set(RANGE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/range")
  set(RANGE_DESIGN_FILE_LIST "${RANGE_RTL_ROOT}/design.flist")
  set(RANGE_DESIGN_TOP "top")
  set(RANGE_SOURCES
    ${RANGE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${RANGE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${RANGE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${RANGE_DESIGN_TOP}")
  set(SOURCES "${RANGE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
