// SPDX-License-Identifier: Apache-2.0

module top #(
    parameter bit ENABLE = 1
) (
    input logic clk,
    output logic [7:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [7:0] counter;
  assign out = counter;

  generate
    if (ENABLE) begin : genblk_on
      always_ff @(posedge clk) begin
        counter <= counter + 1;
      end
    end else begin : genblk_off
      assign counter = '0;
    end
  endgenerate
endmodule
