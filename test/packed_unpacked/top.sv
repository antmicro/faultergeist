// SPDX-License-Identifier: Apache-2.0

module top (
    input clk
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif
  (* keep = 1 *)
  reg [31:0] [1:0] packed_counter /*verilator forceable*/;

  (* keep = 1 *)
  reg [31:0] unpacked_counter [1:0]/*verilator forceable*/;

  always @(posedge clk) begin
    packed_counter <= packed_counter + 1;
    unpacked_counter[0] <= unpacked_counter[0] + 1;
    unpacked_counter[1] <= unpacked_counter[1] + 1;
  end
endmodule
