// SPDX-License-Identifier: Apache-2.0

module top #(
    parameter int ROWS = 2,
    parameter int COLS = 2,
    parameter int STRIDE = 8
) (
    input logic clk,
    output logic [STRIDE*ROWS*COLS-1:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [STRIDE*ROWS*COLS-1:0] q = 0;
  assign out = q;

  genvar row, col;
  generate
    for (row = 0; row < ROWS; row++) begin : genblk_row
      for (col = 0; col < COLS; col++) begin : genblk_col
        logic [7:0] acc;
        always @(posedge clk) begin
          acc <= acc + 8'(row * 16 + 2 * col) + 8'd1;
          q[STRIDE*(row*COLS + col) +: STRIDE] <= acc;
        end
      end
    end
  endgenerate
endmodule
