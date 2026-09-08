macro(define_veer_testbench_vars)
  set(VEER_ROOT  "${PROJECT_SOURCE_DIR}/third_party/Cores-VeeR-EL2")
  set(VEER_DESIGN_FILE_LIST "${E2E_VEER_DIR}/veer.flist")
  set(VEER_DESIGN_TOP "el2_veer_wrapper")
  set(VEER_SOURCES ${VEER_ROOT}/snapshots/dual/common_defines.vh ${VEER_ROOT}/design/include/el2_def.sv ${VEER_ROOT}/snapshots/dual/el2_pdef.vh ${VEER_ROOT}/testbench/tb_top_pkg.sv ${VEER_ROOT}/testbench/tb_top.sv ${VEER_ROOT}/testbench/ahb_sif.sv ${VEER_ROOT}/testbench/jtagdpi/jtagdpi.sv ${VEER_ROOT}/testbench/ahb_lite_2to1_mux.sv ${VEER_ROOT}/testbench/ahb_lsu_dma_bridge.sv ${VEER_ROOT}/testbench/axi4_mux/axi_crossbar_wrap_2x1.v ${VEER_ROOT}/testbench/axi4_mux/arbiter.v ${VEER_ROOT}/testbench/axi4_mux/axi_crossbar_addr.v ${VEER_ROOT}/testbench/axi4_mux/axi_crossbar_rd.v ${VEER_ROOT}/testbench/axi4_mux/axi_crossbar.v ${VEER_ROOT}/testbench/axi4_mux/axi_crossbar_wr.v ${VEER_ROOT}/testbench/axi4_mux/axi_register_rd.v ${VEER_ROOT}/testbench/axi4_mux/axi_register_wr.v ${VEER_ROOT}/testbench/axi4_mux/priority_encoder.v ${VEER_ROOT}/testbench/test_tb_top.cpp ${VEER_ROOT}/testbench/jtagdpi/jtagdpi.c ${VEER_ROOT}/testbench/tcp_server/tcp_server.c)

  set(VERILATOR_EXTRA_ARGS -CFLAGS "-I${VEER_ROOT}/testbench/jtagdpi/ -I${VEER_ROOT}/testbench/tcp_server/ -std=c++14" --coverage-max-width 20000 --top-module tb_top --autoflush --assert -Wno-IMPLICITSTATIC -Wno-TIMESCALEMOD -Wno-ASCRANGE -Wno-CASEINCOMPLETE -Wno-INITIALDLY -Wno-WIDTH -Wno-UNOPTFLAT -Wno-SIDEEFFECT -Wno-LATCH -fno-table -I${VEER_ROOT}/snapshots/dual -I${VEER_ROOT}/testbench/axi4_mux -I${VEER_ROOT}/testbench -f ${VEER_ROOT}/testbench/flist)

  set(RTL_ROOT "${VEER_ROOT}")
  set(DESIGN_TOP "${VEER_DESIGN_TOP}")
  set(DESIGN_FILE_LIST "${VEER_DESIGN_FILE_LIST}")
  set(SOURCES ${VEER_SOURCES})
endmacro()
