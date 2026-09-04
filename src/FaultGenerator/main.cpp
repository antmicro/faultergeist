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

#include "DesignInfo/CellAreaInfoParser.h"
#include "DesignInfo/Liberty.h"
#include "DesignInfo/Placement.h"
#include "FaultCampaignWriter.h"
#include "FaultEvent.h"
#include "FaultEventsSignalFormatter.h"
#include "FaultStrategy/FaultStrategy.h"
#include "GlobalOpts.h"
#include "LogUtils.h"
#include "Signal.h"
#include "SignalCollector/SignalCollector.h"
#include "Utils.h"
#include "VltConfigWriter.h"

#include <filesystem>
#include <future>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>

struct TaskInput {
    std::shared_ptr<FaultStrategy> strategy;
    std::span<const Signal> signals;
    const FaultCampaignWriter& writer;
    std::filesystem::path output_file;
};

std::vector<TaskInput> generate_tasks(
    const std::shared_ptr<FaultStrategy> strategy,
    const std::vector<Signal>& signals,
    const FaultCampaignWriter& writer,
    const std::string& root_path,
    std::uint64_t seed,
    std::uint64_t count
) {
    std::mt19937_64 seed_generator{seed};
    std::uniform_int_distribution<std::uint32_t> dist;
    std::vector<TaskInput> result;
    result.reserve(count);

    for (int i = 0; i < count; i++) {
        FaultStrategy::Config new_config = {
            .num_of_events = strategy->config.num_of_events,
            .seed = dist(seed_generator),
            .simulation_time = strategy->config.simulation_time,
            .thread_number = 1
            // TODO: since we don't want to have more threads than user
            // allowed, when we generate many campaigns in parallel, individual
            // generation will be performed each on single thread
            // This is the branch where we generate multiple campaigns, and in that case we want any
            // particular campaign to run on one thread, because other campaigns take other cores.
            // We could do it smarter, for example we could have a central
            // scheduler that schedules the jobs. For now this is unnecessary.
        };
        std::stringstream campaign_output_filename;
        campaign_output_filename << root_path << "/fault_campaign_" << new_config.seed << ".csv";
        result.emplace_back(
            strategy->copy_with(new_config), signals, writer, campaign_output_filename.str()
        );
    }
    return result;
}

void generate_single_campaign(const TaskInput& input) {
    LOG(INFO) << "call generate_single_campaign";
    try {
        const std::vector<FaultEvent> fault_events = input.strategy->generate(input.signals);
        input.writer.write(input.output_file, fault_events);
        LOG(INFO) << "generate_single_campaign succeeded";
    } catch (...) {
        SEE_CHECK(false) << "generate_single_campaign failed";
    }
}

void create_directory(std::string_view path) {
    using namespace std::filesystem;
    if (exists(path) && is_directory(path)) {
        return;
    }
    std::stringstream ss;
    try {
        if (create_directories(path)) {
            return;
        }
        SEE_CHECK(false) << "Cannot create directory '" << path << "'";
    } catch (std::exception& e) {
        LOG(INFO) << e.what();
        SEE_CHECK(false) << "Cannot create directory '" << path << "'";
    }
}

void generate_campaigns(const GlobalOpts& opts, const std::vector<Signal>& signals) {
    std::string prefix_path = combineSignalPath(opts.sig_path_prefix, opts.top_instance);
    FaultCampaignWriter::FaultFormatter formatter{FaultEventsSignalFormatter(prefix_path, signals)};
    FaultCampaignWriter writer{formatter};

    if (opts.campaign_number == 1) {
        generate_single_campaign({
            .strategy = opts.strategy,
            .signals = signals,
            .writer = writer,
            .output_file = opts.fault_campaign_out,
        });
        return;
    }

    create_directory(opts.fault_campaign_out);
    LOG(INFO) << "call generate_many_campaigns";

    std::vector<TaskInput> tasks = generate_tasks(
        opts.strategy,
        signals,
        writer,
        opts.fault_campaign_out,
        opts.strategy->config.seed,
        opts.campaign_number
    );
    auto num_workers = opts.strategy->config.thread_number;

    std::vector<std::future<void>> workers;

    auto start = tasks.cbegin();
    double distributed = 0.0;
    for (unsigned int i = 0; i < num_workers; ++i) {
        double should_be = ((double)opts.campaign_number * (double)(i + 1)) / num_workers;
        int dist = should_be - distributed;
        auto end = (i == num_workers - 1) ? tasks.cend() : std::next(start, dist);

        workers.push_back(std::async(std::launch::async, [start, end]() {
            std::for_each(start, end, generate_single_campaign);
        }));
        start = end;
        distributed += dist;
    }

    for (auto& fut : workers) {
        fut.wait();
    }
}

int main(int argc, char* argv[]) {
    const GlobalOpts opts = GlobalOpts::parseCmdArgs(argc, argv);
    const PlacementInfo open_road{};

    Liberty liberty;
    if (opts.cell_area_json_path.empty() && !opts.liberty_paths.empty()) {
        liberty = Liberty(opts.liberty_area_scale, opts.liberty_paths);
    } else if (!opts.cell_area_json_path.empty() && opts.liberty_paths.empty()) {
        liberty = Liberty(opts.cell_area_json_path);
    } else {
        SEE_CHECK(false) << "Missing essential input. Provide either liberty_paths or "
                            "cell_area_json_path (via cmd flags or config.json)";
    }

    SEE_CHECK(opts.campaign_number >= 1) << "Cannot run less than one campaign";

    // IMPORTANT: architecture of this system requires that signals vector is not
    // changed, to not invalidate stored iterators. It must remain `const`
    const std::vector<Signal> signals =
        SignalCollector(
            opts.top_module, opts.top_instance, opts.sig_path_prefix, liberty, open_road
        )
            .collectFromFile(opts.netlist_path);

    if (opts.vlt_config) {
        VltConfigWriter::write(*opts.vlt_config, signals);
    }

    generate_campaigns(opts, signals);
}
