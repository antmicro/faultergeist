# SPDX-License-Identifier: Apache-2.0

add_subdirectory(src)

# Build exec
add_executable(faultergeist-gen main.cpp)
target_link_libraries(faultergeist-gen
  PRIVATE
    FaultGenerationToolLib
    nlohmann_json::nlohmann_json
    absl::log
    absl::check
)

# Include unit tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
