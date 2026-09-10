# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

project(Faultergeist LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_WEXTRA "Treat warnings as errors" OFF)

include(CTest)
include(cmake/third-party.cmake)
if(BUILD_TESTING)
  include(cmake/common.cmake)
  fi_require_e2e_tools()
  add_subdirectory(test/worker)
  add_subdirectory(test/veer)
  add_subdirectory(test/wide)
  add_subdirectory(test/signal_widths)
  add_subdirectory(test/packed_unpacked)
  add_subdirectory(test/structs)
  add_subdirectory(test/bit_range)
  add_subdirectory(test/case_generate)
  add_subdirectory(test/for_generate)
  add_subdirectory(test/generate_instances)
  add_subdirectory(test/if_generate)
  add_subdirectory(test/instance_array)
  add_subdirectory(test/multi_instance)
  add_subdirectory(test/multi_scope_signal)
  add_subdirectory(test/named_proc_block)
  add_subdirectory(test/nested)
  add_subdirectory(test/nested_generate)
  add_subdirectory(test/range)
endif()

add_subdirectory(src/FaultGenerator)
add_subdirectory(src/FaultInjector)

include(cmake/install.cmake)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
