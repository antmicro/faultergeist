// SPDX-License-Identifier: Apache-2.0

module top #(
    parameter int MODE = 0
) (
    input logic clk,
    output logic [7:0] out
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  logic [7:0] acc = 0;
  assign out = acc;

  generate
    case (MODE)
      0: begin : genblk_one
        always_ff @(posedge clk) begin
          acc <= acc + 1;
        end
      end
      default: begin : genblk_acc
        always_ff @(posedge clk) begin
          acc <= acc * 2;
        end
      end
    endcase
  endgenerate
endmodule
