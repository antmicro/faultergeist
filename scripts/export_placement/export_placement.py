# Copyright 2026 Antmicro <antmicro.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
import csv


def export_placement(block, csv_file):
    if block is None:
        raise ValueError("no block loaded")

    dbu = float(block.getDbUnitsPerMicron())

    with open(csv_file, "w", newline="") as f:
        out = csv.writer(f, lineterminator="\n")
        out.writerow(["name", "type", "width", "height", "x", "y"])

        die = block.getDieArea()
        out.writerow([block.getName(), "die", die.dx() / dbu, die.dy() / dbu, 0, 0])

        insts = block.getInsts()
        for inst in insts:
            bbox = inst.getBBox()
            x, y = inst.getOrigin()
            out.writerow([
                inst.getName(),
                inst.getMaster().getName(),
                bbox.getDX() / dbu,
                bbox.getDY() / dbu,
                x / dbu,
                y / dbu,
            ])

    print(f"Wrote {len(insts)} cells")
