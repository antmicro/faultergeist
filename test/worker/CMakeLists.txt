macro(define_worker_testbench_vars)
  set(WORKER_ROOT "${PROJECT_SOURCE_DIR}/test/worker")
  set(WORKER_DESIGN_FILE_LIST "${WORKER_ROOT}/dff_worker.flist")
  set(WORKER_DESIGN_TOP "dff_worker")
  set(WORKER_SOURCES
    ${WORKER_ROOT}/top.v
    ${WORKER_ROOT}/worker.v
    ${WORKER_ROOT}/comb_worker.v
    ${WORKER_ROOT}/dff_worker.v
  )

  set(RTL_ROOT "${WORKER_ROOT}")
  set(DESIGN_TOP "${WORKER_DESIGN_TOP}")
  set(DESIGN_FILE_LIST "${WORKER_DESIGN_FILE_LIST}")
  set(SOURCES ${WORKER_SOURCES})
endmacro()
