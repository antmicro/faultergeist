// SPDX-License-Identifier: Apache-2.0

module leaf (
    input logic clk,
    output logic [7:0] out
);
  always @(posedge clk) begin : leaf_blk
    out <= out + 1;
  end
endmodule

module top (
    input logic clk,
    output logic [7:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  reg [7:0] counter = 0;
  assign out = counter;

  leaf leaf (.clk (clk), .out (counter));
endmodule
