// SPDX-License-Identifier: Apache-2.0

typedef struct packed {
  reg [31:0] id;
  reg [31:0] a;
  reg [31:0] b;
} Request;

typedef struct {
  reg [31:0] id;
  reg [31:0] data;
  reg [31:0] timestamp;
} Response;

module top (
    input clk,
    input Request request,
    input wire req_vld,
    output Response response_out,
    output wire req_rdy,
    output wire resp_vld
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif
  reg [31:0] time_cnt/*verilator forceable*/;
  Response response/*verilator forceable*/;

  assign req_rdy = 1;

  always @(posedge clk) begin
    time_cnt <= time_cnt + 1;
  end

  always @(posedge req_vld) begin
    response.id <= request.id;
    response.timestamp <= time_cnt;
    response.data <= request.a + request.b;
  end
  assign response_out = response;

endmodule
