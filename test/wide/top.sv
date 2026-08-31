// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

module top;
  reg [255:0] wide_reg = 0;
`ifdef FAULT_INJECTION_ENABLE
  Faultergeist fi (`FAULT_INJECTION_CAMPAIGN_FILE);
`endif
  initial begin
    if ($test$plusargs("trace") != 0) begin
      $display("[%0t] Tracing to %s...\n", $time, `VCD_OUTPUT_PATH);
      $dumpfile(`VCD_OUTPUT_PATH);
      $dumpvars();
      $display("[%0t] Model running...\n", $time);
    end

    #2 $finish;
  end
endmodule
