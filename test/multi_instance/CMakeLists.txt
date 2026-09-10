# SPDX-License-Identifier: Apache-2.0

macro(define_multi_instance_testbench_vars)
  set(MULTI_INSTANCE_RTL_ROOT "${PROJECT_SOURCE_DIR}/test/multi_instance")
  set(MULTI_INSTANCE_DESIGN_FILE_LIST "${MULTI_INSTANCE_RTL_ROOT}/design.flist")
  set(MULTI_INSTANCE_DESIGN_TOP "top")
  set(MULTI_INSTANCE_SOURCES
    ${MULTI_INSTANCE_RTL_ROOT}/top.sv
  )

  set(RTL_ROOT "${MULTI_INSTANCE_RTL_ROOT}")
  set(DESIGN_FILE_LIST "${MULTI_INSTANCE_DESIGN_FILE_LIST}")
  set(DESIGN_TOP "${MULTI_INSTANCE_DESIGN_TOP}")
  set(SOURCES "${MULTI_INSTANCE_SOURCES}")
  set(SYNTH_HDL_FRONTEND "slang")
endmacro()
