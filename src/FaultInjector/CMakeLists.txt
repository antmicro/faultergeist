# SPDX-License-Identifier: Apache-2.0

if(ENABLE_WEXTRA)
  add_compile_options(-Werror -Wall -Wextra)
endif()

add_library(faultergeist-inject STATIC FaultInjector.cpp src/EventParser.cpp)

target_include_directories(faultergeist-inject PRIVATE
  src/
)

# Include unit tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
