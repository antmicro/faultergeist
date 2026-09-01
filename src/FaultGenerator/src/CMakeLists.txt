# SPDX-License-Identifier: Apache-2.0

add_library(FaultGenerationToolLib
  FaultStrategy/Bendel.cpp
  FaultStrategy/Factory.cpp
  FaultStrategy/Random.cpp
  FaultStrategy/Weibull.cpp

  SignalCollector/IsFlipFlopPredicate.cpp
  SignalCollector/SignalCollector.cpp
  SignalCollector/SlangModuleCollector.cpp
  SignalCollector/YosysModuleCollector.cpp

  FaultCampaignWriter.cpp
  GlobalOpts.cpp
  Liberty.cpp
  LibertyParser.cpp
  OpenROADParser.cpp
  PlacementInfo.cpp
  FaultEventsSignalFormatter.cpp
)

target_include_directories(FaultGenerationToolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FaultGenerationToolLib
  PUBLIC
    absl::log
    absl::log_initialize
    absl::check
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,absl::log_flags>"
    mp-units::mp-units
    nlohmann_json::nlohmann_json
  PRIVATE
    absl::flags
    slang::slang
    absl::flags_parse
    absl::strings
)
