# SPDX-License-Identifier: Apache-2.0

add_library(FaultGenerationToolLib
  FaultCampaignWriter.cpp
  FaultStrategyBendel.cpp
  FaultStrategyFactory.cpp
  FaultStrategyRandom.cpp
  FaultStrategyWeibull.cpp
  GlobalOpts.cpp
  Liberty.cpp
  LibertyParser.cpp
  OpenROADParser.cpp
  PlacementInfo.cpp
  SignalCollector/IsFlipFlopPredicate.cpp
  SignalCollector/SignalCollector.cpp
  SignalCollector/SlangModuleCollector.cpp
  SignalCollector/YosysModuleCollector.cpp
  FaultEventsSignalFormatter.cpp
)

target_include_directories(FaultGenerationToolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FaultGenerationToolLib
  PUBLIC
    absl::log
    absl::log_initialize
    absl::check
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,absl::log_flags>"
    nlohmann_json::nlohmann_json
  PRIVATE
    absl::flags
    slang::slang
    absl::flags_parse
    absl::strings
)
