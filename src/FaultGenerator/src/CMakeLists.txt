# SPDX-License-Identifier: Apache-2.0

add_library(FaultGenerationToolLib
  DesignInfo/CellAreaInfoParser.cpp
  DesignInfo/Liberty.cpp
  DesignInfo/LibertyParser.cpp
  DesignInfo/OpenROADParser.cpp
  DesignInfo/Placement.cpp

  FaultStrategy/Bendel.cpp
  FaultStrategy/Exhaustive.cpp
  FaultStrategy/Factory.cpp
  FaultStrategy/Random.cpp
  FaultStrategy/Weibull.cpp

  SignalCollector/IsFlipFlopPredicate.cpp
  SignalCollector/SignalCollector.cpp
  SignalCollector/SlangModuleCollector.cpp
  SignalCollector/YosysModuleCollector.cpp

  FaultCampaignWriter.cpp
  FaultEventsSignalFormatter.cpp
  GlobalOpts.cpp
  VltConfigWriter.cpp
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
