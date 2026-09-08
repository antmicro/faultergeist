# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED FI_VERILATOR_COVERAGE)
  message(FATAL_ERROR "FI_VERILATOR_COVERAGE is required")
endif()
if(NOT DEFINED FI_COVERAGE_DAT)
  message(FATAL_ERROR "FI_COVERAGE_DAT is required")
endif()
if(NOT DEFINED FI_LOG)
  message(FATAL_ERROR "FI_LOG is required")
endif()

if(NOT EXISTS "${FI_COVERAGE_DAT}")
  message(FATAL_ERROR "Coverage file not found: ${FI_COVERAGE_DAT}")
endif()

get_filename_component(_log_dir "${FI_LOG}" DIRECTORY)
file(MAKE_DIRECTORY "${_log_dir}")

execute_process(
  COMMAND "${FI_VERILATOR_COVERAGE}" "${FI_COVERAGE_DAT}"
  OUTPUT_FILE "${FI_LOG}"
  ERROR_FILE "${FI_LOG}.err"
  RESULT_VARIABLE _result
)

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "verilator_coverage failed with exit code ${_result}")
endif()

if(DEFINED FI_GOLDEN_FILE)
  if(NOT EXISTS "${FI_GOLDEN_FILE}")
    message(FATAL_ERROR
      "Coverage golden file not found: ${FI_GOLDEN_FILE}\n"
      "Generate it with: cp '${FI_LOG}' '${FI_GOLDEN_FILE}'"
    )
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${FI_GOLDEN_FILE}" "${FI_LOG}"
    RESULT_VARIABLE _cmp_result
  )

  if(NOT _cmp_result EQUAL 0)
    message(FATAL_ERROR
      "Coverage summary does not match golden file.\n"
      "  Expected: ${FI_GOLDEN_FILE}\n"
      "  Actual:   ${FI_LOG}\n"
      "Update with: cp '${FI_LOG}' '${FI_GOLDEN_FILE}'"
    )
  endif()
endif()
