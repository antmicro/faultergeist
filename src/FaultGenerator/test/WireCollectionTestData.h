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

#pragma once

#include <string_view>

namespace wire_collection_test {

inline constexpr std::string_view json = R"json({
"modules": {
  "my_cell": {
    "cells": {
    }
  },
  "worker": {
    "cells": {
        "dff_worker0.counter[0]$my_cell": {
            "type": "my_cell",
            "parameters": {
            },
            "attributes": {
                "hdlname": "dff_worker0 counter$dff"
            }
        }
    },
    "netnames": {
        "dff_worker0.counter": {
            "bits": [ 0 ],
            "attributes": {
                "hdlname": "dff_worker0 counter"
            }
        },
        "range_wire": {
            "bits": [ 40, 41, 42, 43, 44, 45, 46, 47 ],
            "offset": 8,
            "attributes": {
                "hdlname": "range_wire"
            }
        },
        "synth_wire": {
            "bits": [ 21 ],
            "attributes": {
                "keep": "1"
            }
        },
        "clk": {
            "bits": [ 37 ]
        },
        "out": {
            "bits": [ 3, 4, 5, 6 ]
        }
    },
    "ports": {
        "clk": {
            "direction": "input",
            "bits": [ 37 ]
        },
        "out": {
            "direction": "output",
            "bits": [ 3, 4, 5, 6 ]
        }
    }
  }
}
})json";

inline constexpr std::string_view verilog = R"verilog(
module my_cell;
endmodule

module worker(
    input  wire       clk,
    output wire [3:0] out
);
    (* hdlname = "dff_worker0 counter" *)
    wire \dff_worker0.counter ;

    (* hdlname = "range_wire" *)
    wire [15:8] range_wire;

    (* keep = 1 *)
    wire synth_wire;

    (* hdlname = "dff_worker0 counter$dff" *)
    my_cell \dff_worker0.counter[0]$my_cell ();
endmodule
)verilog";

}  // namespace wire_collection_test
