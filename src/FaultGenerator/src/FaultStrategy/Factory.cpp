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

#include "Factory.h"

#include "Bendel.h"
#include "FaultStrategy.h"
#include "LogUtils.h"
#include "Random.h"
#include "UnitUtils.h"
#include "Weibull.h"

#include <nlohmann/json.hpp>

#include <memory>

namespace {
template <typename Quantity, typename Unit>
void readQuantity(const nlohmann::json& json, const char* name, Quantity& quantity, Unit unit) {
    if (const auto value = json.find(name); value != json.end()) {
        quantity = value->get<double>() * unit;
    }
}

template <typename Quantity>
void readMaxTime(const nlohmann::json& json, Quantity& quantity) {
    auto parsed = unit::parseQuantity(json["max_time"].get<std::string_view>());
    if (parsed) {
        auto [value, unit] = *parsed;
        if (auto result = unit::normalizeSimTime(value, unit)) {
            quantity = *result;
        }
    }
}

template <typename Stream, typename Pred>
void removeMatchingStreams(std::vector<Stream>& streams, Pred p) {
    streams.erase(std::remove_if(streams.begin(), streams.end(), p), streams.end());
}

}  // namespace

void from_json(const nlohmann::json& json, WeibullConfig::Stream& stream) {
    stream = {};
    readQuantity(json, "let", stream.let, unit::MeV * unit::cm2 / unit::mg);
    readQuantity(json, "flux_phi", stream.flux_phi, inverse(unit::s * unit::cm2));

    readMaxTime(json, stream.max_time);
}

void from_json(const nlohmann::json& json, WeibullConfig& config) {
    config = {};
    readQuantity(json, "let_threshold", config.let_threshold, unit::MeV * unit::cm2 / unit::mg);
    readQuantity(json, "width", config.width, unit::MeV * unit::cm2 / unit::mg);
    readQuantity(json, "shape_parameter", config.shape_parameter, unit::one);
    readQuantity(json, "limiting_cross_section", config.limiting_cross_section, unit::cm2);

    config.streams = json.value("streams", config.streams);
    removeMatchingStreams(config.streams, [&config](auto stream) {
        return !stream.isValid(config.let_threshold);
    });
}

void from_json(const nlohmann::json& json, BendelConfig::Stream& stream) {
    stream = {};
    stream.name = json.value("name", stream.name);
    readQuantity(json, "energy", stream.energy, unit::MeV);
    readQuantity(json, "flux_phi", stream.flux_phi, inverse(unit::s * unit::cm2));
    readMaxTime(json, stream.max_time);
}

void from_json(const nlohmann::json& json, BendelConfig& config) {
    config = {};
    readQuantity(json, "A", config.A, unit::MeV);
    readQuantity(json, "B", config.B, unit::MeV);

    config.streams = json.value("streams", config.streams);
    removeMatchingStreams(config.streams, [&config](auto stream) {
        return !stream.isValid(config.A);
    });
}

std::shared_ptr<FaultStrategy> FaultStrategyFactory::buildFromJson(
    const FaultStrategy::Config& config,
    const nlohmann::json& model_config
) {
    std::string_view model_name = model_config.at("name").get<std::string_view>();
    if (model_name == "random") {
        // TODO Here maybe warn that random model received params, when it doesn't expect to
        LOG(INFO) << "Parsed random model from json";
        return std::make_shared<RandomStrategy>(config);
    } else if (model_name == "weibull") {
        const auto weibull_config = model_config.at("params").get<WeibullConfig>();
        LOG(INFO) << "Parsed weibull model from json";
        return std::make_shared<WeibullStrategy>(config, weibull_config);
    } else if (model_name == "bendel") {
        const auto bendel_config = model_config.at("params").get<BendelConfig>();
        LOG(INFO) << "Parsed bendel model from json";
        return std::make_shared<BendelStrategy>(config, bendel_config);
    } else {
        SEE_CHECK(false) << "Unknown model: " << model_name << "\n";
    }
}

std::shared_ptr<FaultStrategy> FaultStrategyFactory::defaultStrategy(
    const FaultStrategy::Config& config
) {
    LOG(INFO) << "Selecting default (random) model";
    return std::make_shared<RandomStrategy>(RandomStrategy{config});
}
