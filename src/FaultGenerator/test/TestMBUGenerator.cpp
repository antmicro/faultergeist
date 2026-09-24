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

#include "DesignInfo/PlacementParser.h"
#include "FaultStrategy/MBUGenerator.h"
#include "FaultStrategy/PlacementMap.h"
#include "FaultStrategy/Random.h"
#include "FaultStrategy/Runner.h"
#include "Signal.h"

#include <gtest/gtest.h>

#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

Signal signal(std::string name, std::optional<Placement> cell_placement, std::uint32_t width = 1) {
    return Signal{
        Cell{.name = std::move(name), .type = "$dff", .hdlname = "", .width = width},
        "",
        1 * unit::um2,
        cell_placement,
        SignalType::REGISTER
    };
}

Placement placement(double x, double y, double width, double height) {
    return Placement{
        .width = width * unit::um, .height = height * unit::um, .x = x * unit::um, .y = y * unit::um
    };
}

}  // namespace

TEST(PlacementMapTest, FindsPlacedSignals) {
    std::vector<Signal> signals{
        signal("small", placement(0, 0, 2, 2)),
        signal("wide", placement(2, 0, 100, 2)),
        signal("unplaced", std::nullopt),
    };
    PlacementMap map{signals};

    EXPECT_EQ((map[1 * unit::um, 1 * unit::um]), &signals[0]);
    EXPECT_EQ((map[90 * unit::um, 1 * unit::um]), &signals[1]);
    EXPECT_EQ((map[3 * unit::um, 1 * unit::um]), &signals[1]);
    EXPECT_EQ((map[103 * unit::um, 1 * unit::um]), nullptr);
    EXPECT_EQ((map[1 * unit::um, 3 * unit::um]), nullptr);
}

TEST(PlacementMapTest, EmptyAndInvalidPlacements) {
    EXPECT_NO_THROW({
        PlacementMap empty;
        EXPECT_EQ((empty[{0 * unit::um, 0 * unit::um}]), nullptr);
        std::vector<Signal> signals({
            signal("missing", std::nullopt),
            signal("zero", placement(0, 0, 0, 1)),
            signal("negative", placement(0, 0, 1, -1)),
            signal("infinite", placement(std::numeric_limits<double>::infinity(), 0, 1, 1)),
        });
        PlacementMap map{signals};
        EXPECT_EQ((map[0 * unit::um, 0 * unit::um]), nullptr);
    });
}

TEST(PlacementMapTest, MatchesLinearReferenceAcrossShapesAndOverlaps) {
    for (const auto scale : {0.001, 1.0, 1000.0}) {
        std::vector<Signal> signals;
        for (int i = 0; i < 20; ++i) {
            signals.push_back(signal(
                std::to_string(i),
                placement(
                    ((i * 7) % 13 - 6) * scale, (i * 3) % 11 - 5, (i % 5 + 1) * scale, i % 3 + 1
                )
            ));
        }
        PlacementMap map{signals};
        for (int x = -16; x <= 24; ++x) {
            for (int y = -14; y <= 20; ++y) {
                const Position pos{x * 0.5 * scale * unit::um, y * 0.5 * unit::um};
                const Signal* expected = nullptr;
                for (const auto& candidate : signals) {
                    const auto& p = *candidate.cell_placement;
                    if (p.x < pos.x && pos.x < p.x + p.width && p.y < pos.y &&
                        pos.y < p.y + p.height) {
                        expected = &candidate;
                        break;
                    }
                }
                EXPECT_EQ(map[pos], expected) << "scale=" << scale << " x=" << x << " y=" << y;
            }
        }
        EXPECT_EQ(
            (map[std::numeric_limits<double>::quiet_NaN() * unit::um, 0 * unit::um]), nullptr
        );
    }
}

TEST(MBUGeneratorTest, DisabledDoesNotConsumeRandomState) {
    MBUGenerator generator;
    FaultStrategy::RandomGen actual{42};
    FaultStrategy::RandomGen expected{42};
    Signal primary = signal("primary", std::nullopt);

    EXPECT_EQ(generator.generateSecondaryFault(actual, primary), nullptr);
    EXPECT_EQ(actual.random_generator(), expected.random_generator());
}

TEST(MBUGeneratorTest, UnplacedPrimaryDoesNotConsumeRandomState) {
    std::vector<Signal> signals{signal("unplaced", std::nullopt)};
    MBUGenerator generator{signals, 1 * unit::um};
    FaultStrategy::RandomGen actual{42};
    FaultStrategy::RandomGen expected{42};

    EXPECT_EQ(generator.generateSecondaryFault(actual, signals[0]), nullptr);
    EXPECT_EQ(actual.random_generator(), expected.random_generator());
}

TEST(MBUGeneratorTest, Approximately95PercentOfHitsAreInsideRadius) {
    constexpr int samples = 50000;
    const Position center{10 * unit::um, -10 * unit::um};
    const Signal primary = signal("primary", placement(9.5, -10.5, 1, 1));
    for (const auto radius : {0.25 * unit::um, 4.0 * unit::um}) {
        SCOPED_TRACE(::testing::Message() << "radius=" << radius);
        const auto cell_size = radius / 50;

        std::vector<Signal> signals;
        signals.reserve(200 * 200);
        for (int x = -100; x < 100; ++x) {
            for (int y = -100; y < 100; ++y) {
                signals.push_back(signal(
                    "cell",
                    Placement{
                        .width = cell_size,
                        .height = cell_size,
                        .x = center.x + x * cell_size,
                        .y = center.y + y * cell_size,
                    }
                ));
            }
        }

        const MBUGenerator generator{signals, radius};
        FaultStrategy::RandomGen random{42};

        int hits = 0;
        int inside = 0;
        int misses = 0;
        for (int i = 0; i < samples; ++i) {
            const auto* hit = generator.generateSecondaryFault(random, primary);
            if (!hit) {
                ++misses;
                continue;
            }
            ++hits;
            const auto& p = *hit->cell_placement;
            const auto dx = p.x + p.width / 2 - center.x;
            const auto dy = p.y + p.height / 2 - center.y;
            inside += dx * dx + dy * dy <= radius * radius;
        }
        // Cover +/-2 radii on each axis; do not silently discard a wide distribution's tails.
        ASSERT_LE(misses, 10) << "hits=" << hits << " samples=" << samples;
        // Cell centers approximate sample positions within sqrt(2)/100 of the radius.
        // Allow one percentage point for this discretization and sampling variation.
        EXPECT_NEAR(static_cast<double>(inside) / hits, 0.95, 0.01)
            << "inside=" << inside << " outside=" << hits - inside << " hits=" << hits
            << " misses=" << misses;
    }
}

TEST(MBUGeneratorTest, SecondaryBitsUseTheirOwnCellWidth) {
    const FaultStrategy::Config config{
        .num_of_events = 1000,
        .seed = 42,
        .simulation_time = 100 * unit::ns,
        .thread_number = 2,
    };
    for (const auto secondary_width : {1u, 31u}) {
        std::vector<Signal> signals{
            signal("secondary", placement(-100, -100, 200, 200), secondary_width),
            signal("primary", placement(-1, -1, 2, 2), secondary_width == 1 ? 31 : 1),
        };
        const MBUGenerator generator{signals, 0.01 * unit::um};
        const auto check = [&](const std::vector<FaultEvent>& events) {
            ASSERT_FALSE(events.empty());
            ASSERT_EQ(events.size() % 2, 0);
            bool reached_high_bit = false;
            bool different_widths = false;
            for (std::size_t i = 1; i < events.size(); i += 2) {
                const auto& secondary = events[i];
                EXPECT_EQ(&*secondary.it, &signals[0]);
                EXPECT_LE(secondary.bit_index, secondary.it->cell.width);
                if (events[i - 1].it->cell.width != secondary.it->cell.width) {
                    different_widths = true;
                    reached_high_bit |= secondary.bit_index > 1;
                }
            }
            EXPECT_TRUE(different_widths);
            if (secondary_width > 1) {
                EXPECT_TRUE(reached_high_bit);
            }
        };
        RandomStrategy random{config};
        check(random.generate(generator, signals));
        const std::vector<int> streams{0};
        const auto next_time = [](const Signal&, int, FaultStrategy::RandomGen&) -> unit::TIME {
            return 1 * unit::ns;
        };
        const auto max_time = [&](int) { return config.simulation_time; };
        FaultStrategyRunner runner{
            next_time,
            max_time,
            config,
            generator,
            std::span<const int>{streams},
            std::span<const Signal>{signals}
        };
        check(runner.generateInParallelByTimeSlice());
    }
}

TEST(PlacementMapTest, LookupFromExamplePlacementCSV) {
    const auto path = std::string(TEST_DATA_DIR) + "/example_placement_data.csv.in";
    const auto placements = PlacementParser::parse(path);
    ASSERT_TRUE(placements.getDeviceInfo());
    const auto& die = *placements.getDeviceInfo();
    ASSERT_GT(die.width, unit::DIST::zero());
    ASSERT_GT(die.height, unit::DIST::zero());

    std::ifstream input(path);
    ASSERT_TRUE(input);
    std::string line;
    ASSERT_TRUE(std::getline(input, line));  // Column header.
    ASSERT_TRUE(std::getline(input, line));  // Die placement.
    std::vector<Signal> signals;
    while (std::getline(input, line)) {
        auto name = line.substr(0, line.find(','));
        std::erase(name, '\\');
        const auto cell = placements.getCellPlacement(name);
        ASSERT_TRUE(cell) << name;
        signals.push_back(signal(name, cell));
    }
    ASSERT_FALSE(signals.empty());

    for (const auto& cell : signals) {
        SCOPED_TRACE(cell.cell.name);
        const auto& p = *cell.cell_placement;
        ASSERT_GT(p.width, unit::DIST::zero());
        ASSERT_GT(p.height, unit::DIST::zero());
        ASSERT_GE(p.x, die.x);
        ASSERT_GE(p.y, die.y);
        ASSERT_LE(p.x + p.width, die.x + die.width);
        ASSERT_LE(p.y + p.height, die.y + die.height);
    }

    PlacementMap map{signals};
    std::size_t hits = 0;
    std::size_t misses = 0;
    const auto check = [&](Position pos) {
        const Signal* expected = nullptr;
        for (const auto& candidate : signals) {
            const auto& p = *candidate.cell_placement;
            if (p.x < pos.x && pos.x < p.x + p.width && p.y < pos.y && pos.y < p.y + p.height) {
                expected = &candidate;
                break;
            }
        }
        expected ? ++hits : ++misses;
        EXPECT_EQ(map[pos], expected) << "x=" << pos.x << " y=" << pos.y;
    };
    for (const auto& cell : signals) {
        SCOPED_TRACE(cell.cell.name);
        const auto& p = *cell.cell_placement;
        check({p.x + p.width / 2, p.y + p.height / 2});
        for (double x : {0.25, 0.75}) {
            for (double y : {0.25, 0.75}) {
                const Position pos{p.x + p.width * x, p.y + p.height * y};
                const auto* found = map[pos];
                ASSERT_NE(found, nullptr) << "x=" << pos.x << " y=" << pos.y;
                if (found != &cell) {
                    SCOPED_TRACE(found->cell.name);
                    const auto& secondary = *found->cell_placement;
                    EXPECT_TRUE(
                        secondary.x < pos.x && pos.x < secondary.x + secondary.width &&
                        secondary.y < pos.y && pos.y < secondary.y + secondary.height
                    ) << "x="
                      << pos.x << " y=" << pos.y;
                }
            }
        }
    }
    for (int x = -1; x <= 101; ++x) {
        for (int y = -1; y <= 101; ++y) {
            check({die.x + die.width * x / 100, die.y + die.height * y / 100});
        }
    }

    EXPECT_EQ(hits, 1839u);
    EXPECT_EQ(misses, 8993u);
}
