// SPDX-License-Identifier: Apache-2.0

module top #(
    parameter int NUM = 4
) (
    input logic clk,
    output logic [8*NUM-1:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [8*NUM-1:0] q = 0;
  assign out = q;

  genvar i;
  generate
    for (i = 0; i < NUM; i++) begin : genblk
      logic [7:0] acc = 0;
      always_ff @(posedge clk) begin
        acc <= acc + 8'(2 * i) + 8'd1;
        q[8 * i +: 8] <= acc;
      end
    end
  endgenerate
endmodule
