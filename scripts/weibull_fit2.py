#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">3.12,<3.14"
# dependencies = ["numpy", "matplotlib"]
# ///

import numpy as np
import matplotlib.pyplot as plt

# =============================================================================

class Cell:
    def __init__(self, area):
        self.area = area

        # # TRAD/TI/R1RW0416DSB/1346/ESA/LG/1409
        # # https://escies.org/download/webDocumentFile?id=63660
        # self.weibull_L0 = 1.09   * 1e5  # [Mev * cm^2 / mg]
        # self.weibull_W  = 39.25  * 1e5  # [Mev * cm^2 / mg]
        # self.weibull_s  = 1.116
        # self.weibull_g0 = 0.284  * 1e-4 # [cm^2 / bit]

        # # HRX/SEE/0073
        # # https://escies.org/download/webDocumentFile?id=847
        # self.weibull_L0 = 0.7249   * 1e5  # [Mev * cm^2 / mg]
        # self.weibull_W  = 16.26   * 1e5  # [Mev * cm^2 / mg]
        # self.weibull_s  = 1.02
        # self.weibull_g0 = 3.3e-8   * 1e-4 # [cm^2 / bit]

        # # "An empirical model for predicting proton induced upset"
        # # 62256R
        self.weibull_L0 = 1.60   * 1e5  # [Mev * cm^2 / mg]
        self.weibull_W  = 20.0   * 1e5  # [Mev * cm^2 / mg]
        self.weibull_s  = 1.65
        self.weibull_g0 = 0.640  * 1e-4 # [cm^2 / bit]

class Stream:
    def __init__(self, model, phi, let_or_energy, theta=0.0):
        self.model          = model
        self.flux_phi       = phi
        self.flux_theta     = theta
        self.let_or_energy  = let_or_energy

    def __call__(self, cell):
        cos = np.cos(np.deg2rad(self.flux_theta))

        if self.model == "weibull":
            # Weibull curve
            g = cell.weibull_g0 * (1.0 - np.exp(-np.power((self.let_or_energy / cos - cell.weibull_L0) / cell.weibull_W, cell.weibull_s)))
        else:
            assert False, self.model

        # Lambda for exponential distribution
        h = g * self.flux_phi * cell.area * cos

        p  = np.random.rand()
        dt = -np.log(1 - p) / h
        return dt

# =============================================================================

def run_simulation(**kw):

    # Artificially correct cell area for bit count and num_cells
    # Cell area given
    if "cell_area" in kw:
        cell_area = kw["cell_area"] * (kw["bit_count"] / kw["num_cells"])
    # Device area given
    elif "device_area" in kw:
        cell_area = kw["device_area"] / kw["num_cells"]
    else:
        assert False

    cell = Cell(
        cell_area,
    )

    if "let" in kw:
        let_or_energy = kw["let"]
    if "energy" in kw:
        let_or_energy = kw["energy"]

    streams = [Stream(
        kw["model"],
        kw["flux_phi"],
        let_or_energy,
        kw.get("flux_theta", 0.0),
    )]

    # Initialize
    event_time = [
        [stream(cell) for stream in streams]
        for i in range(kw["num_cells"])
    ]

    history    = [list() for i in range(kw["num_cells"])]
    curr_time  = 0.0
    total_hits = 0

    # Run
    while True:
        # Next event
        time_list = [min(l) for l in event_time]
        indx_list = [l.index(t) for t, l in zip(time_list, event_time)]

        cell_id   = time_list.index(min(time_list))
        stream_id = indx_list[cell_id]

        prev_time = curr_time
        curr_time = event_time[cell_id][stream_id]

        # Finish
        if curr_time >= kw["max_time"]:
            break

        # Store event history
        history[cell_id].append((curr_time, stream_id))
        total_hits += 1

        # Schedule next one
        event_time[cell_id][stream_id] += streams[stream_id](cell)

    # Visualize
    vis = np.zeros((kw["num_cells"], 512, 3), dtype=np.uint8)
    for i, events in enumerate(history):
        y = i
        for j, hit in events:
            x = int(j * vis.shape[1] / kw["max_time"])
            vis[y, x, :] = [
                (255, 0, 0),
                (0, 255, 0),
                (0, 0, 255),
            ][hit]

    return total_hits, vis

# =============================================================================

def main():

    # # TRAD/TI/R1RW0416DSB/1346/ESA/LG/1409
    # # https://escies.org/download/webDocumentFile?id=63660
    # cases = [
    #     # {
    #     #     "name":       "1",
    #     #     "let":        67.7   * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   9.15e3 * 1e4,  # [s^-1 * cm^-2]
    #     #     "max_time":   1094,          # [s]
    #     # },
    #     {
    #         "name":       "2",
    #         "let":        67.7   * 1e5,
    #         "flux_phi":   1.01e3 * 1e4,
    #         "max_time":   996,
    #     },
    #     {
    #         "name":       "3",
    #         "let":        67.7   * 1e5,
    #         "flux_phi":   1.04e3 * 1e4,
    #         "max_time":   409,
    #     },
    #     {
    #         "name":       "4",
    #         "let":        67.7   * 1e5,
    #         "flux_phi":   1.05e3 * 1e4,
    #         "max_time":   399,
    #     },
    #     {
    #         "name":       "5",
    #         "let":        67.7   * 1e5,
    #         "flux_phi":   5.04e2 * 1e4,
    #         "max_time":   166,
    #     },
    #     {
    #         "name":       "6",
    #         "let":        40.4   * 1e5,
    #         "flux_phi":   1.01e3 * 1e4,
    #         "max_time":   536,
    #     },
    #     {
    #         "name":       "7",
    #         "let":        40.4   * 1e5,
    #         "flux_phi":   1.01e3 * 1e4,
    #         "max_time":   551,
    #     },

    #     {
    #         "name":       "8",
    #         "let":        32.6   * 1e5,
    #         "flux_phi":   1.58e3 * 1e4,
    #         "max_time":   417,
    #     },
    #     {
    #         "name":       "9",
    #         "let":        32.6   * 1e5,
    #         "flux_phi":   1.51e3 * 1e4,
    #         "max_time":   411,
    #     },
    #     {
    #         "name":       "10",
    #         "let":        32.6   * 1e5,
    #         "flux_phi":   1.45e3 * 1e4,
    #         "max_time":   12,
    #     },
    #     {
    #         "name":       "11",
    #         "let":        20.4   * 1e5,
    #         "flux_phi":   2.00e3 * 1e4,
    #         "max_time":   433,
    #     },
    #     {
    #         "name":       "12",
    #         "let":        20.4   * 1e5,
    #         "flux_phi":   2.05e3 * 1e4,
    #         "max_time":   452,
    #     },
    #     {
    #         "name":       "13",
    #         "let":        10.2   * 1e5,
    #         "flux_phi":   2.32e3 * 1e4,
    #         "max_time":   433,
    #     },
    #     {
    #         "name":       "14",
    #         "let":        10.2   * 1e5,
    #         "flux_phi":   2.77e3 * 1e4,
    #         "max_time":   636,
    #     },
    #     {
    #         "name":       "15",
    #         "let":        3.0    * 1e5,
    #         "flux_phi":   5.03e3 * 1e4,
    #         "max_time":   201,
    #     },
    #     {
    #         "name":       "16",
    #         "let":        3.0    * 1e5,
    #         "flux_phi":   5.11e3 * 1e4,
    #         "max_time":   197,
    #     },
    #     {
    #         "name":       "17",
    #         "let":        1.1    * 1e5,
    #         "flux_phi":   7.60e3 * 1e4,
    #         "max_time":   133,
    #     },
    #     {
    #         "name":       "18",
    #         "let":        1.1    * 1e5,
    #         "flux_phi":   8.17e3 * 1e4,
    #         "max_time":   124,
    #     },
    #     {
    #         "name":       "19",
    #         "let":        32.6   * 1e5,
    #         "flux_phi":   9.99e3 * 1e4,
    #         "max_time":   102,
    #     },
    #     {
    #         "name":       "20",
    #         "let":        32.6   * 1e5,
    #         "flux_phi":   5.17e1 * 1e4,
    #         "max_time":   1275,
    #     },

    # ]

    params = {
         "model":        "weibull",
         "bit_count":    256000 * 16, # 4MB
         "cell_area":    0.25e-6,     # [mm^2] (tuned manually!)
         "num_cells":    64,          # Artificial. 1 cell represents N device cells
    }

    # ......................................

    # # HRX/SEE/0073
    # # https://escies.org/download/webDocumentFile?id=847

    cases = [
        {
            "name":       "run108",
            "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
            "flux_phi":   6993.0 * 1e4,  # [s^-1 * cm^-2]
            "flux_theta": 0.0,
            "max_time":   143,           # [s]
            "seu_count":  435,
        },
        {
            "name":       "run107",
            "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
            "flux_phi":   7143.0 * 1e4,  # [s^-1 * cm^-2]
            "flux_theta": 0.0,
            "max_time":   140,           # [s]
            "seu_count":  429,
        },
    #     # {
    #     #     "name":       "run109",
    #     #     "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   4975.0 * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 45.0,
    #     #     "max_time":   201,           # [s]
    #     #     "seu_count":  743,
    #     # },
    #     # {
    #     #     "name":       "run110",
    #     #     "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   3390.0 * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 60.0,
    #     #     "max_time":   295,           # [s]
    #     #     "seu_count":  1914,
    #     # },

    #     {
    #         "name":       "run73",
    #         "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
    #         "flux_phi":   489.0  * 1e4,  # [s^-1 * cm^-2]
    #         "flux_theta": 0.0,
    #         "max_time":   159,           # [s]
    #     },
    #     {
    #         "name":       "run72",
    #         "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
    #         "flux_phi":   81.0   * 1e4,  # [s^-1 * cm^-2]
    #         "flux_theta": 0.0,
    #         "max_time":   291,           # [s]
    #     },
    #     # {
    #     #     "name":       "run74",
    #     #     "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   835.0  * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 45.0,
    #     #     "max_time":   160,           # [s]
    #     # },
    #     # {
    #     #     "name":       "run75",
    #     #     "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   594.0  * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 60.0,
    #     #     "max_time":   237,           # [s]
    #     # },

    #     {
    #         "name":       "run23",
    #         "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
    #         "flux_phi":   419.0  * 1e4,  # [s^-1 * cm^-2]
    #         "flux_theta": 0.0,
    #         "max_time":   225,           # [s]
    #     },
    #     {
    #         "name":       "run22",
    #         "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
    #         "flux_phi":   511.0  * 1e4,  # [s^-1 * cm^-2]
    #         "flux_theta": 0.0,
    #         "max_time":   186,           # [s]
    #     },
    #     # {
    #     #     "name":       "run25",
    #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   242.0  * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 45.0,
    #     #     "max_time":   248,           # [s]
    #     # },
    #     # {
    #     #     "name":       "run24",
    #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   244.0  * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 45.0,
    #     #     "max_time":   248,           # [s]
    #     # },
    #     # {
    #     #     "name":       "run26",
    #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
    #     #     "flux_phi":   142.0  * 1e4,  # [s^-1 * cm^-2]
    #     #     "flux_theta": 60.0,
    #     #     "max_time":   341,           # [s]
    #     # },
    ]

    # ......................................

    for i, case in enumerate(cases):
        name = case.get("name", f"#{i}")

        kw = dict(params.items())
        kw.update(case)

        data = []
        for j in range(10):
            total_hits, vis = run_simulation(**kw)
            data.append(total_hits)

        if "seu_count" in case:
            s = f"vs. {case['seu_count']}"
        else:
            s = ""

        print(name, np.min(data), np.average(data), np.max(data), s)

if __name__ == "__main__":
    main()
