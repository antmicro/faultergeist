# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)


FetchContent_Declare(
    absl
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20250512.0
)
FetchContent_MakeAvailable(absl)


set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
FetchContent_Declare(
  slang
  GIT_REPOSITORY https://github.com/MikePopoloski/slang.git
  GIT_TAG 0cc882855a21cc96e48847c64acd80d3bd5e02e1
)
FetchContent_MakeAvailable(slang)

FetchContent_Declare(
    mp-units
    GIT_REPOSITORY https://github.com/mpusz/mp-units.git
    GIT_TAG v2.5.0
)
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "Enable contract checking" FORCE)
FetchContent_GetProperties(mp-units)
if(NOT mp-units_POPULATED)
    FetchContent_Populate(mp-units)

    add_subdirectory(
        ${mp-units_SOURCE_DIR}/src
        ${mp-units_BINARY_DIR}
    )
endif()


if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(googletest)
endif()
