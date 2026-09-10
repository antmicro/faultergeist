// SPDX-License-Identifier: Apache-2.0

module leaf (
    input logic clk,
    output logic [7:0] out
);
  logic [7:0] counter;
  assign out = counter;
  always @(posedge clk) begin : leaf_blk
    counter <= counter + 1;
  end
endmodule

module top (
    input logic clk,
    output logic [31:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  leaf leaf [3:0] (.clk(clk), .out(out));
endmodule
