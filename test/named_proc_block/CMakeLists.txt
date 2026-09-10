# SPDX-License-Identifier: Apache-2.0

macro(define_named_proc_block_testbench_vars)
  set(NAMED_PROC_BLOCK_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/named_proc_block")
  set(NAMED_PROC_BLOCK_DESIGN_FILE_LIST "${NAMED_PROC_BLOCK_RTL_ROOT}/design.flist")
  set(NAMED_PROC_BLOCK_DESIGN_TOP "top")
  set(NAMED_PROC_BLOCK_SOURCES
    ${NAMED_PROC_BLOCK_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${NAMED_PROC_BLOCK_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${NAMED_PROC_BLOCK_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${NAMED_PROC_BLOCK_DESIGN_TOP}")
  set(SOURCES "${NAMED_PROC_BLOCK_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
