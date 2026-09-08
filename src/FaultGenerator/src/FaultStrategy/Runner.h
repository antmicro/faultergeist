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

#include "FaultEvent.h"
#include "FaultStrategy.h"
#include "LogUtils.h"
#include "ScheduledEvent.h"
#include "Signal.h"
#include "UnitUtils.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <functional>
#include <future>
#include <queue>
#include <random>
#include <vector>

template <typename Generator, typename Stream>
concept EventTimeGenerator = requires(
    const Generator& generator,
    const Signal& signal,
    const Stream& stream,
    FaultStrategy::RandomGen& random
) {
    { generator(signal, stream, random) } -> std::convertible_to<unit::TIME>;
};

template <typename Calculator, typename Stream>
concept MaxTimeCalculator = requires(const Calculator& calculator, const Stream& stream) {
    { calculator(stream) } -> std::convertible_to<unit::SIM_TIME>;
};

template <
    // Concept of a stream is dependent on Model used
    typename Stream,

    // EventTimeGeneratorT is a functional that returns generated time that should
    // pass between particular fault events.
    EventTimeGenerator<Stream> EventTimeGeneratorT,

    // MaxTimeCalculatorT is a functional that returns maximal the stream should
    // be alive. FaultStrategyRunner will take minimum of that and global simulation_time.
    MaxTimeCalculator<Stream> MaxTimeCalculatorT>
class FaultStrategyRunner {
    const EventTimeGeneratorT& evTimeGenerator;
    const MaxTimeCalculatorT& maxTimeCalc;
    const FaultStrategy::Config& config;
    std::span<const Stream> streams;
    std::span<const Signal> signals;
    std::vector<unit::SIM_TIME> max_times;

   public:
    explicit FaultStrategyRunner(
        const EventTimeGeneratorT& eventTimeGenerator,
        const MaxTimeCalculatorT& maxTimeCalc,
        const FaultStrategy::Config& config,
        std::span<const Stream> streams,
        std::span<const Signal> signals
    )
        : evTimeGenerator(eventTimeGenerator),
          maxTimeCalc(maxTimeCalc),
          config(config),
          streams(streams),
          signals(signals) {
        SEE_CHECK(streams.size() > 0) << "No streams read";
        max_times.reserve(streams.size());
        for (std::size_t i = 0; i < streams.size(); i++) {
            max_times.push_back(std::min(config.simulation_time, maxTimeCalc(streams[i])));
            VLOG(2) << "Calculated Stream " << i
                    << " max time: " << std::format("{}", max_times[i]);
        }
    }

    std::pair<unit::TIME, unit::SIM_TIME> scheduleWorkTime(std::size_t worker_id) const {
        auto max_elem = std::max_element(max_times.begin(), max_times.end());
        const auto max_time = max_elem->numerical_value_in(unit::SIM_TIME::unit);
        const auto boundary = [&](std::size_t worker) {
            return (max_time / config.thread_number * worker +
                    max_time % config.thread_number * worker / config.thread_number) *
                   unit::SIM_TIME::unit;
        };
        auto begin = boundary(worker_id);
        auto end = boundary(worker_id + 1);
        VLOG(2) << "Worker #" << worker_id << " [out of " << config.thread_number
                << "] will work on {" << std::format("{},{}", begin, end) << "} "
                << std::format("(max_time: {})", *max_elem);
        return {begin, end};
    }

    std::vector<FaultEvent> generateInParallelByTimeSlice() const {
        FaultStrategy::RandomGen gen = FaultStrategy::RandomGen(config.seed);
        if (config.thread_number == 1) {
            // if there is only one thread allowed, don't spawn another one
            const auto [begin, end] = scheduleWorkTime(0);
            return generateSingleTimeSlice(gen, begin, end);
        }

        std::vector<std::vector<FaultEvent>> partial_results{config.thread_number};

        std::vector<FaultStrategy::RandomGen> worker_generators;
        worker_generators.reserve(config.thread_number);
        for (std::size_t i = 0; i < config.thread_number; ++i) {
            worker_generators.emplace_back(gen.random_generator());
        }

        std::vector<std::future<std::size_t>> workers;
        for (std::size_t i = 0; i < config.thread_number; ++i) {
            const auto [begin, end] = scheduleWorkTime(i);
            workers.push_back(std::async(
                std::launch::async,
                [this, i, begin, end, &partial_results, &worker_generators]() {
                    partial_results[i] = generateSingleTimeSlice(worker_generators[i], begin, end);
                    return partial_results[i].size();
                }
            ));
        }

        std::size_t total_events = 0;
        for (auto& fut : workers) {
            total_events += fut.get();
        }

        std::vector<FaultEvent> result{total_events};
        const auto* original_ptr = result.data();
        auto iter = result.begin();
        for (const auto& current : partial_results) {
            assert(original_ptr == result.data());
            iter = std::copy(current.begin(), current.end(), iter);
        }
        return result;
    }

    std::vector<FaultEvent> generateSingleTimeSlice(
        FaultStrategy::RandomGen& worker_gen,
        unit::TIME begin_time,
        unit::SIM_TIME end_time
    ) const {
        VLOG(1) << "Strategy generating on time slice from "
                << std::format("{} to {}", begin_time, end_time);
        std::vector<FaultEvent> result;
        std::uniform_int_distribution<std::uint32_t> int_dist;

        std::priority_queue<
            ScheduledEvent,
            std::vector<ScheduledEvent>,
            std::greater<ScheduledEvent>>
            event_queue;

        for (std::size_t stream_id = 0; stream_id < streams.size(); ++stream_id) {
            for (std::size_t signal_id = 0; signal_id < signals.size(); ++signal_id) {
                event_queue.emplace(
                    begin_time +
                        evTimeGenerator(signals[signal_id], streams[stream_id], worker_gen),
                    signal_id,
                    stream_id
                );
            }
        }
        VLOG(2) << "Events added to the queue: " << event_queue.size();

        while (!event_queue.empty()) {
            const ScheduledEvent next = event_queue.top();
            event_queue.pop();

            auto end = std::min(max_times[next.stream_id], end_time);
            if (next.time >= end || next.time >= unit::MAX_SIM_TIME) {
                continue;
            }
            if (config.tooManyEventsGenerated(result.size())) {
                LOG(WARNING) << "Amount of generated fault exceeded number specified in"
                                " the config. Stoping generation.";
                // this exposes that in the previous implementation config limited number of events
                // per stream, not overall as it was supposed to
                break;
            }

            const auto& signal = signals[next.signal_id];
            const auto& stream = streams[next.stream_id];
            result.emplace_back(
                signals.begin() + next.signal_id,
                unit::toSimTime(next.time),
                /*signal_path=*/"",
                int_dist(
                    worker_gen.random_generator,
                    std::uniform_int_distribution<std::uint32_t>::param_type{0, signal.cell.width}
                ),
                faultEventType(signal.type)
            );

            // Schedule next one
            event_queue.emplace(
                next.time + evTimeGenerator(signal, stream, worker_gen),
                next.signal_id,
                next.stream_id
            );
        }

        VLOG(1) << "Weibull strategy generated " << result.size() << " faults";
        return result;
    }
};
