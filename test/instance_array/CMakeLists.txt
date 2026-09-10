# SPDX-License-Identifier: Apache-2.0

macro(define_instance_array_testbench_vars)
  set(INSTANCE_ARRAY_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/instance_array")
  set(INSTANCE_ARRAY_DESIGN_FILE_LIST "${INSTANCE_ARRAY_RTL_ROOT}/design.flist")
  set(INSTANCE_ARRAY_DESIGN_TOP "top")
  set(INSTANCE_ARRAY_SOURCES
    ${INSTANCE_ARRAY_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${INSTANCE_ARRAY_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${INSTANCE_ARRAY_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${INSTANCE_ARRAY_DESIGN_TOP}")
  set(SOURCES "${INSTANCE_ARRAY_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
