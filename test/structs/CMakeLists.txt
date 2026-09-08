# SPDX-License-Identifier: Apache-2.0

macro(define_structs_testbench_vars)
  set(STRUCTS_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/structs")
  set(STRUCTS_DESIGN_FILE_LIST "${STRUCTS_RTL_ROOT}/design.flist")
  set(STRUCTS_DESIGN_TOP "top")
  set(STRUCTS_SOURCES
    ${STRUCTS_RTL_ROOT}/top.sv
    ${PROJECT_SOURCE_DIR}/test/e2e/coverage/SimMain.cpp
  )

  set(RTL_ROOT "${STRUCTS_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${STRUCTS_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${STRUCTS_DESIGN_TOP}")
  set(SOURCES "${STRUCTS_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
