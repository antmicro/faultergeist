// SPDX-License-Identifier: Apache-2.0

module leaf (
    input clk,
    input wire req_vld,
    input wire resp_rdy,
    input wire [31:0] req,
    output reg req_rdy,
    output reg resp_vld,
    output reg [31:0] resp,
    output reg [31:0] counter
);
  assign req_rdy = req_vld;
  assign resp_vld = req_vld;

  always @(posedge clk) begin
    if (req_vld) begin
      resp <= req;
      counter <= counter + 1;
    end else begin
      resp <= 0;
    end
  end
endmodule

module top (
    input clk,
    input wire in_cond_a,
    input wire in_cond_b,
    input wire in_resp_rdy,
    input wire [31:0] in_req,
    output reg out_req_rdy,
    output reg out_resp_vld,
    output reg [31:0] out_resp,
    output reg [31:0] out_counter
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  wire req_vld = in_cond_a & in_cond_b;
  leaf leaf(
    .clk (clk),
    .req_vld (req_vld),
    .resp_rdy (in_resp_rdy),
    .req (in_req),
    .req_rdy (out_req_rdy),
    .resp_vld (out_resp_vld),
    .resp (out_resp),
    .counter (out_counter)
  );

endmodule
