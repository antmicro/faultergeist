// SPDX-License-Identifier: Apache-2.0

module leaf (
    input logic clk,
    output logic [7:0] out
);
  logic [7:0] counter = 0;
  assign out = counter;

  always @(posedge clk) begin
    counter <= counter + 1;
  end
endmodule

module top #(
    parameter int N = 3
) (
    input logic clk,
    output logic [8*N-1:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  genvar i;
  generate
    for (i = 0; i < N; i++) begin : genblk_inst
      leaf leaf (.clk(clk), .out(out[8*i +: 8]));
    end
  endgenerate
endmodule
