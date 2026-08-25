# SPDX-License-Identifier: Apache-2.0

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

add_library(FaultGeneratorTestMain STATIC FaultGeneratorTestMain.cpp)
target_link_libraries(FaultGeneratorTestMain
  PUBLIC
    GTest::gtest
    FaultGenerationToolLib
  PRIVATE
    absl::flags_parse
)

#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestFaultStrategyWeibull TestFaultStrategyWeibull.cpp)
target_link_libraries(TestFaultStrategyWeibull
  PRIVATE
    FaultGeneratorTestMain
)
add_test(NAME unit.generation.faultStrategy.weibull COMMAND TestFaultStrategyWeibull)

#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestConfigReading TestConfigReading.cpp)
target_link_libraries(TestConfigReading
  PRIVATE
    FaultGeneratorTestMain
    absl::flags
    absl::flags_parse
    absl::strings
)
add_test(NAME unit.generation.gtest.configReading  COMMAND TestConfigReading)


#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestFaultStrategyBendel TestFaultStrategyBendel.cpp)
target_link_libraries(TestFaultStrategyBendel PRIVATE FaultGeneratorTestMain)
add_test(NAME unit.generation.faultStrategy.bendel COMMAND TestFaultStrategyBendel)

add_executable(TestLibertyParsing TestLibertyParsing.cpp)
target_link_libraries(TestLibertyParsing PRIVATE FaultGeneratorTestMain)
target_compile_definitions(TestLibertyParsing PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
add_test(NAME unit.generation.gtest.libertyParsing COMMAND TestLibertyParsing)

#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestOpenROADParsing TestOpenROADParsing.cpp)
target_link_libraries(TestOpenROADParsing PRIVATE FaultGeneratorTestMain)
target_compile_definitions(TestOpenROADParsing PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
add_test(NAME unit.generation.gtest.openroadParsing COMMAND TestOpenROADParsing)

#==============================================================================
#==============================================================================
#==============================================================================
set(WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/work")
set(JSON_NETLIST "${WORK_DIR}/netlist.json")
set(VERILOG_NETLIST "${WORK_DIR}/netlist.v")
add_executable(TestSignalCollector TestSignalCollector.cpp)
if(FI_E2E_TOOLS_FOUND)
  block()
    define_worker_testbench_vars()
    fi_add_yosys_json(TestSignalCollector_worker_netlists)
  endblock()
  add_dependencies(TestSignalCollector TestSignalCollector_worker_netlists)
endif()
target_link_libraries(TestSignalCollector
  PRIVATE
    FaultGeneratorTestMain
    slang::slang
)
target_compile_definitions(TestSignalCollector PRIVATE
  FI_E2E_TOOLS_FOUND=$<BOOL:${FI_E2E_TOOLS_FOUND}>
  WORKER_JSON_NETLIST="${JSON_NETLIST}"
  WORKER_VERILOG_NETLIST="${VERILOG_NETLIST}"
)
add_test(NAME unit.generation.gtest.signalCollector COMMAND TestSignalCollector)

#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestFaultEventsSignalFormatter TestFaultEventsSignalFormatter.cpp)
target_link_libraries(TestFaultEventsSignalFormatter
  PRIVATE
    FaultGeneratorTestMain
)
add_test(NAME unit.generation.gtest.signalGroupingFaultFormatter COMMAND TestFaultEventsSignalFormatter)

#==============================================================================
#==============================================================================
#==============================================================================
add_executable(TestFaultStrategyDeterminism TestFaultStrategyDeterminism.cpp)
target_link_libraries(TestFaultStrategyDeterminism PRIVATE FaultGeneratorTestMain)
target_compile_definitions(TestFaultStrategyDeterminism PRIVATE
  TEST_DATA_DIR="${TEST_DATA_DIR}"
)
add_test(NAME unit.generation.faultStrategy.generationDeterministic COMMAND TestFaultStrategyDeterminism)

#==============================================================================
#==============================================================================
#==============================================================================
# Binary for testing LibertyParser
add_executable(LibertyParserBin LibertyParser.cpp)
target_link_libraries(LibertyParserBin
  PRIVATE
    FaultGenerationToolLib
    absl::flags
    absl::flags_parse
    absl::strings
)
add_test(NAME unit.generation.libertyParserOnExample
  COMMAND
  LibertyParserBin --input_path "${TEST_DATA_DIR}/example.lib"
)
