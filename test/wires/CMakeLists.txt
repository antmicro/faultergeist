# SPDX-License-Identifier: Apache-2.0

macro(define_wires_testbench_vars)
  set(WIRES_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/wires")
  set(WIRES_DESIGN_FILE_LIST "${WIRES_RTL_ROOT}/design.flist")
  set(WIRES_DESIGN_TOP "top")
  set(WIRES_SOURCES
    ${WIRES_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${WIRES_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${WIRES_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${WIRES_DESIGN_TOP}")
  set(SOURCES "${WIRES_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
