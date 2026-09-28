# SPDX-License-Identifier: Apache-2.0

cmake_policy(SET CMP0007 NEW)

if(NOT DEFINED FI_FIRST_FILE OR NOT DEFINED FI_SECOND_FILE)
  message(FATAL_ERROR "FI_FIRST_FILE and FI_SECOND_FILE must be defined")
endif()

file(STRINGS "${FI_FIRST_FILE}" _first_lines)
file(STRINGS "${FI_SECOND_FILE}" _second_lines)
list(SORT _first_lines)
list(SORT _second_lines)

if(NOT "${_first_lines}" STREQUAL "${_second_lines}")
  message(FATAL_ERROR
    "Files differ after sorting lines:\n"
    "  ${FI_FIRST_FILE}\n"
    "  ${FI_SECOND_FILE}"
  )
endif()
