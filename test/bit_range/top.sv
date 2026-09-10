// SPDX-License-Identifier: Apache-2.0

/* verilator coverage_off */
module top (
    input clk,
    output logic [15:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

/* verilator coverage_on */
  logic [31:16] counter;
`ifdef SYNTHESIS
  always_ff @(posedge clk) begin
    counter <= counter + 1;
  end
`endif

  logic [39:32][3:2] packed_counter;
`ifdef SYNTHESIS
  always_ff @(posedge clk) begin
    packed_counter <= packed_counter + 1;
  end
`endif

  logic [31:16] unpacked_counter [3:2];
`ifdef SYNTHESIS
  always_ff @(posedge clk) begin
    unpacked_counter[2] <= unpacked_counter[2] + 1;
    unpacked_counter[3] <= unpacked_counter[3] + 1;
  end
`endif

  logic [31:16] sized_unpacked_counter [4];
`ifdef SYNTHESIS
  always_ff @(posedge clk) begin
    sized_unpacked_counter[0] <= sized_unpacked_counter[0] + 1;
    sized_unpacked_counter[1] <= sized_unpacked_counter[1] + 1;
    sized_unpacked_counter[2] <= sized_unpacked_counter[2] + 1;
    sized_unpacked_counter[3] <= sized_unpacked_counter[3] + 1;
  end
`endif

  logic [31:16] matrix_counter [3:2][3:2];
`ifdef SYNTHESIS
  always_ff @(posedge clk) begin
    matrix_counter[2][2] <= matrix_counter[2][2] + 1;
    matrix_counter[2][3] <= matrix_counter[2][3] + 1;
    matrix_counter[3][2] <= matrix_counter[3][2] + 1;
    matrix_counter[3][3] <= matrix_counter[3][3] + 1;
  end
`endif
/* verilator coverage_off */

  assign out = counter + packed_counter + unpacked_counter[2] + unpacked_counter[3] +
               sized_unpacked_counter[0] + sized_unpacked_counter[1] +
               sized_unpacked_counter[2] + sized_unpacked_counter[3] +
               matrix_counter[2][2] + matrix_counter[2][3] + matrix_counter[3][2] +
               matrix_counter[3][3] + 1;
endmodule
