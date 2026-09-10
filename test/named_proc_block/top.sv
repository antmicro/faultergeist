// SPDX-License-Identifier: Apache-2.0

module top (
    input logic clk,
    output logic [7:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [7:0] counter = 0;
  assign out = counter;

  always_ff @(posedge clk) begin : proc_blk
    counter <= counter + 1;
  end
endmodule
