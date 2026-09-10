// SPDX-License-Identifier: Apache-2.0

module top (
    input logic clk,
    output logic [31:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [31:0] buffer = 0;
  assign out = buffer;

  logic [31:0] acc = 0;
  logic enable = 0;

  always_ff @(posedge clk) begin
    acc <= acc + 32'd1;
    enable <= ~enable;
  end

  // Lower bits with simple flop.
  always_ff @(posedge clk) begin
    buffer[15:0] <= acc[15:0];
  end

  // High bits with enabled flop.
  always_ff @(posedge clk) begin
    if (enable) buffer[31:16] <= acc[31:16];
  end
endmodule
