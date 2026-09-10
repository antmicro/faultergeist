# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "Enable contract checking" FORCE)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20250512.0
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  slang
  GIT_REPOSITORY https://github.com/MikePopoloski/slang.git
  GIT_TAG 0cc882855a21cc96e48847c64acd80d3bd5e02e1
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  mp-units
  GIT_REPOSITORY https://github.com/mpusz/mp-units.git
  GIT_TAG v2.5.0
  SOURCE_SUBDIR src
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(json absl slang mp-units)

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(googletest)
endif()
