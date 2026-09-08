// SPDX-License-Identifier: Apache-2.0

module top(input clk);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  (* keep = 1 *)
  reg single_bit /* verilator forceable */;

  (* keep = 1 *)
  reg [15:0] counter /* verilator forceable */;

  (* keep = 1 *)
  reg [122:0] wide_counter /* verilator forceable */;

  always @(posedge clk) begin
    counter <= counter + 1;
    wide_counter <= wide_counter + 1;
    single_bit <= single_bit + 1;
  end
endmodule
