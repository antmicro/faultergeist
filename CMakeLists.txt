# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.25)

project(Faultergeist LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/third-party.cmake)
include(CTest)

if(BUILD_TESTING)
  include(cmake/common.cmake)
  fi_require_e2e_tools()
  add_subdirectory(test/worker)
  add_subdirectory(test/veer)
endif()

add_subdirectory(src/FaultGenerator)
add_subdirectory(src/FaultInjector)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
