// SPDX-License-Identifier: Apache-2.0

/* verilator coverage_off */

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

module processor (
    input clk,
    input Request request,
    input logic req_vld,
    output logic [31:0] resp_out,
    output logic req_rdy,
    output logic resp_vld
);

/* verilator coverage_on */

  reg [31:0] time_cnt = 0;
  Request acc = 0;
  Response response;

  // Expose `response` members as wires as unpacked struct members are not included in coverage.
  wire [31:0] response_id_toggle = response.id;
  wire [31:0] response_data_toggle = response.data;
  wire [31:0] response_timestamp_toggle = response.timestamp;

/* verilator coverage_off */

  assign req_rdy = 1;
  assign resp_vld = req_vld;

  always @(posedge clk) begin
    time_cnt <= time_cnt + 1;

    if (req_vld) begin
      acc.id <= acc.id + request.id;
      acc.a <= acc.a + request.a;
      acc.b <= acc.b + request.b;

      response.id <= request.id;
      response.timestamp <= time_cnt;
      response.data <= acc.id + acc.a + acc.b;

      resp_out <= response.id + response.timestamp + response.data;
    end
  end

endmodule

module top (
    input clk,
    input Request request,
    input logic req_vld,
    output logic [31:0] resp_out,
    output logic req_rdy,
    output logic resp_vld
);
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif

  processor proc (.*);

endmodule

/* verilator coverage_on */
