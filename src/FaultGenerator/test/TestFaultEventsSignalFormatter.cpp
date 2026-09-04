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

#include "FaultCampaignWriter.h"
#include "FaultEvent.h"
#include "FaultEventsSignalFormatter.h"
#include "Signal.h"
#include "TestUtils.h"
#include "Utils.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

TEST(FaultEventsSignalFormatter, NormalTest) {
    const std::vector<Signal> pre_synth_signals = {
        createSignal("top.worker", "counter", 32),
        createSignal("top.worker", "resp", 32),
    };

    // `hdlname` format is determined in SignalCollector.
    // See signal collector tests for expected format.
    const std::vector<Signal> post_synth_signals = {
        createSignal("top.worker", "counter[0]", 1),
        createSignal("top.worker", "counter[6]", 1),
        createSignal("top.worker", "counter[22]", 1),
        createSignal("top.worker", "counter[24]", 1),
        createSignal("top.worker", "counter[25]", 1),
        createSignal("top.worker", "resp[1]", 1),
        createSignal("top.worker", "resp[3]", 1),
        createSignal("top.worker", "resp[18]", 1),
        createSignal("top.worker", "resp[21]", 1),
        createSignal("top.worker", "resp[26]", 1),
    };

    const std::vector<FaultEvent> pre_synth_events = {
        createFromSignal(pre_synth_signals, 0, 1, 6),
        createFromSignal(pre_synth_signals, 0, 2, 22),
        createFromSignal(pre_synth_signals, 1, 4, 21),
        createFromSignal(pre_synth_signals, 1, 4, 18),
        createFromSignal(pre_synth_signals, 1, 6, 26),
        createFromSignal(pre_synth_signals, 0, 6, 25),
        createFromSignal(pre_synth_signals, 1, 8, 3),
        createFromSignal(pre_synth_signals, 1, 8, 1),
        createFromSignal(pre_synth_signals, 0, 8, 0),
        createFromSignal(pre_synth_signals, 0, 10, 24),
    };
    const std::vector<FaultEvent> post_synth_events = {
        createFromSignal(post_synth_signals, 1, 1, 0),
        createFromSignal(post_synth_signals, 2, 2, 0),
        createFromSignal(post_synth_signals, 8, 4, 0),
        createFromSignal(post_synth_signals, 7, 4, 0),
        createFromSignal(post_synth_signals, 9, 6, 0),
        createFromSignal(post_synth_signals, 4, 6, 0),
        createFromSignal(post_synth_signals, 6, 8, 0),
        createFromSignal(post_synth_signals, 5, 8, 0),
        createFromSignal(post_synth_signals, 0, 8, 0),
        createFromSignal(post_synth_signals, 3, 10, 0),
    };

    const std::string_view prefix_path = "top";

    FaultCampaignWriter::FaultFormatter pre_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, pre_synth_signals}
    );
    FaultCampaignWriter::FaultFormatter post_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, post_synth_signals}
    );

    std::stringstream pre_synth_sstream;
    std::stringstream post_synth_sstream;

    FaultCampaignWriter(pre_synth_formatter).write(pre_synth_sstream, pre_synth_events);
    FaultCampaignWriter(post_synth_formatter).write(post_synth_sstream, post_synth_events);

    testStreams(pre_synth_sstream, post_synth_sstream);
};

TEST(FaultEventsSignalFormatter, BracketFalsePositives) {
    const std::vector<Signal> false_positives_signals = {
        createSignal("top.worker", "counter\\[123\\]", 1),
        createSignal("top.worker", "resp\\[123\\]", 1),
    };

    const std::vector<FaultEvent> events = {
        createFromSignal(false_positives_signals, 0, 1, 0),
        createFromSignal(false_positives_signals, 0, 2, 0),
        createFromSignal(false_positives_signals, 0, 4, 0),
        createFromSignal(false_positives_signals, 1, 4, 0),
        createFromSignal(false_positives_signals, 1, 6, 0),
        createFromSignal(false_positives_signals, 0, 6, 0),
        createFromSignal(false_positives_signals, 0, 8, 0),
        createFromSignal(false_positives_signals, 1, 8, 0),
        createFromSignal(false_positives_signals, 0, 9, 0),
        createFromSignal(false_positives_signals, 0, 10, 0),
    };

    const std::string_view prefix_path = "top";

    FaultCampaignWriter::FaultFormatter false_positive_formatter(
        FaultEventsSignalFormatter{prefix_path, false_positives_signals}
    );
    const std::vector<std::string> signal_paths = {
        combineSignalPath(
            false_positives_signals[0].path_prefix, false_positives_signals[0].cell.getPath()
        ),
        combineSignalPath(
            false_positives_signals[1].path_prefix, false_positives_signals[1].cell.getPath()
        ),
    };
    FaultCampaignWriter::FaultFormatter null_formatter([&false_positives_signals,
                                                        &signal_paths](FaultEvent event) {
        event.signal_path =
            signal_paths[event.it - std::span<const Signal>(false_positives_signals).begin()];
        return event;
    });

    std::stringstream false_positive_sstream;
    std::stringstream null_sstream;

    FaultCampaignWriter(false_positive_formatter).write(false_positive_sstream, events);
    FaultCampaignWriter(null_formatter).write(null_sstream, events);

    testStreams(false_positive_sstream, null_sstream);
}

TEST(FaultEventsSignalFormatter, BothKindsOfBrackets) {
    const std::vector<Signal> pre_synth_signals = {
        createSignal("top.worker", "counter\\[123\\]", 32),
        createSignal("top.worker", "resp\\[123\\]", 32),
    };

    const std::vector<Signal> post_synth_signals = {
        createSignal("top.worker", "counter\\[123\\][0]", 1),
        createSignal("top.worker", "counter\\[123\\][6]", 1),
        createSignal("top.worker", "counter\\[123\\][22]", 1),
        createSignal("top.worker", "counter\\[123\\][24]", 1),
        createSignal("top.worker", "counter\\[123\\][25]", 1),
        createSignal("top.worker", "resp\\[123\\][1]", 1),
        createSignal("top.worker", "resp\\[123\\][3]", 1),
        createSignal("top.worker", "resp\\[123\\][18]", 1),
        createSignal("top.worker", "resp\\[123\\][21]", 1),
        createSignal("top.worker", "resp\\[123\\][26]", 1),
    };

    const std::vector<FaultEvent> pre_synth_events = {
        createFromSignal(pre_synth_signals, 0, 1, 6),
        createFromSignal(pre_synth_signals, 0, 2, 22),
        createFromSignal(pre_synth_signals, 1, 4, 21),
        createFromSignal(pre_synth_signals, 1, 4, 18),
        createFromSignal(pre_synth_signals, 1, 6, 26),
        createFromSignal(pre_synth_signals, 0, 6, 25),
        createFromSignal(pre_synth_signals, 1, 8, 3),
        createFromSignal(pre_synth_signals, 1, 8, 1),
        createFromSignal(pre_synth_signals, 0, 8, 0),
        createFromSignal(pre_synth_signals, 0, 10, 24),
    };
    const std::vector<FaultEvent> post_synth_events = {
        createFromSignal(post_synth_signals, 1, 1, 0),
        createFromSignal(post_synth_signals, 2, 2, 0),
        createFromSignal(post_synth_signals, 8, 4, 0),
        createFromSignal(post_synth_signals, 7, 4, 0),
        createFromSignal(post_synth_signals, 9, 6, 0),
        createFromSignal(post_synth_signals, 4, 6, 0),
        createFromSignal(post_synth_signals, 6, 8, 0),
        createFromSignal(post_synth_signals, 5, 8, 0),
        createFromSignal(post_synth_signals, 0, 8, 0),
        createFromSignal(post_synth_signals, 3, 10, 0),
    };

    const std::string_view prefix_path = "top";

    FaultCampaignWriter::FaultFormatter pre_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, pre_synth_signals}
    );
    FaultCampaignWriter::FaultFormatter post_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, post_synth_signals}
    );

    std::stringstream pre_synth_sstream;
    std::stringstream post_synth_sstream;

    FaultCampaignWriter(pre_synth_formatter).write(pre_synth_sstream, pre_synth_events);
    FaultCampaignWriter(post_synth_formatter).write(post_synth_sstream, post_synth_events);

    testStreams(pre_synth_sstream, post_synth_sstream);
}

TEST(FaultEventsSignalFormatter, SignalsWithHdlname) {
    // `hdlname` here will be always empty due to it being added during synthesis steps.
    const std::vector<Signal> pre_synth_signals = {
        createSignal("top.worker", "counter\\[123\\]", 32),
        createSignal("top.worker", "resp", 32),
    };

    const std::vector<Signal> post_synth_signals = {
        createSignal("top.worker", "counter\\[123\\][0]", 1, "worker counter\\[123\\]$dff"),
        createSignal("top.worker", "counter\\[123\\][6]", 1, "worker counter\\[123\\]$dff"),
        createSignal("top.worker", "counter\\[123\\][22]", 1, "worker counter\\[123\\]$dff"),
        createSignal("top.worker", "counter\\[123\\][24]", 1, "worker counter\\[123\\]$dff"),
        createSignal("top.worker", "counter\\[123\\][25]", 1),  // Intentionally empty `hdlname`.
        createSignal("top.worker", "resp[1]", 1, "worker resp$dff"),
        createSignal("top.worker", "resp[3]", 1, "worker resp$dff"),
        createSignal("top.worker", "resp[18]", 1, "worker resp$dff"),
        createSignal("top.worker", "resp[21]", 1, "worker resp$dff"),
        createSignal("top.worker", "resp[26]", 1, "worker resp$dff"),
    };

    const std::vector<FaultEvent> pre_synth_events = {
        createFromSignal(pre_synth_signals, 0, 1, 6),
        createFromSignal(pre_synth_signals, 0, 2, 22),
        createFromSignal(pre_synth_signals, 1, 4, 21),
        createFromSignal(pre_synth_signals, 1, 4, 18),
        createFromSignal(pre_synth_signals, 1, 6, 26),
        createFromSignal(pre_synth_signals, 0, 6, 25),
        createFromSignal(pre_synth_signals, 1, 8, 3),
        createFromSignal(pre_synth_signals, 1, 8, 1),
        createFromSignal(pre_synth_signals, 0, 8, 0),
        createFromSignal(pre_synth_signals, 0, 10, 24),
    };
    const std::vector<FaultEvent> post_synth_events = {
        createFromSignal(post_synth_signals, 1, 1, 0),
        createFromSignal(post_synth_signals, 2, 2, 0),
        createFromSignal(post_synth_signals, 8, 4, 0),
        createFromSignal(post_synth_signals, 7, 4, 0),
        createFromSignal(post_synth_signals, 9, 6, 0),
        createFromSignal(post_synth_signals, 4, 6, 0),
        createFromSignal(post_synth_signals, 6, 8, 0),
        createFromSignal(post_synth_signals, 5, 8, 0),
        createFromSignal(post_synth_signals, 0, 8, 0),
        createFromSignal(post_synth_signals, 3, 10, 0),
    };

    const std::string_view prefix_path = "top";

    FaultCampaignWriter::FaultFormatter pre_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, pre_synth_signals}
    );
    FaultCampaignWriter::FaultFormatter post_synth_formatter(
        FaultEventsSignalFormatter{prefix_path, post_synth_signals}
    );

    std::stringstream pre_synth_sstream;
    std::stringstream post_synth_sstream;

    FaultCampaignWriter(pre_synth_formatter).write(pre_synth_sstream, pre_synth_events);
    FaultCampaignWriter(post_synth_formatter).write(post_synth_sstream, post_synth_events);

    testStreams(pre_synth_sstream, post_synth_sstream);
}
