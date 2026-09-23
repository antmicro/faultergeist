# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED FI_COMMAND)
  message(FATAL_ERROR "FI_COMMAND is required")
endif()
if("${FI_COMMAND}" STREQUAL "")
  message(FATAL_ERROR "FI_COMMAND is empty")
endif()
if(NOT DEFINED FI_LOG)
  message(FATAL_ERROR "FI_LOG is required")
endif()
if(NOT DEFINED FI_WORK_DIR)
  set(FI_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

list(GET FI_COMMAND 0 _fi_executable)
if("${_fi_executable}" STREQUAL "")
  message(FATAL_ERROR "FI_COMMAND has an empty executable: ${FI_COMMAND}")
endif()
if(NOT EXISTS "${_fi_executable}")
  message(FATAL_ERROR "Simulation executable does not exist: ${_fi_executable}\nFull command: ${FI_COMMAND}\nWorking directory: ${FI_WORK_DIR}")
endif()
if(IS_DIRECTORY "${_fi_executable}")
  message(FATAL_ERROR "Simulation executable is a directory: ${_fi_executable}\nFull command: ${FI_COMMAND}\nWorking directory: ${FI_WORK_DIR}")
endif()

get_filename_component(_log_dir "${FI_LOG}" DIRECTORY)
file(MAKE_DIRECTORY "${_log_dir}")

set(_fi_command ${FI_COMMAND})
if(FI_COVERAGE_TESTING)
  set(_fi_command "${CMAKE_COMMAND}" -E env "FI_COVERAGE_TESTING=1" -- ${_fi_command})
endif()

execute_process(
  COMMAND ${_fi_command}
  WORKING_DIRECTORY "${FI_WORK_DIR}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

set(_combined "${_stdout}${_stderr}")
file(WRITE "${FI_LOG}" "${_combined}")

if(NOT FI_EXPECT_FAIL)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Simulation command failed with exit code ${_result}. See ${FI_LOG}")
  endif()
endif()

if(FI_EXPECT_VCD_GOLDENFILE)
  list(GET FI_EXPECT_VCD_GOLDENFILE 0 _vcd_actual)
  list(GET FI_EXPECT_VCD_GOLDENFILE 1 _vcd_expected)

  if(NOT EXISTS "${_vcd_actual}")
    message(FATAL_ERROR "Golden file assertion failed. Actual VCD file does not exist: ${_vcd_actual}")
  endif()
  if(NOT EXISTS "${_vcd_expected}")
    message(FATAL_ERROR "Golden file assertion failed. Expected VCD file does not exist: ${_vcd_expected}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files ${_vcd_expected} ${_vcd_actual}
    RESULT_VARIABLE _expect_vcd_result
  )

  if(NOT _expect_vcd_result EQUAL 0)
    message(FATAL_ERROR "Golden file assertion failed. Expected ${_vcd_expected} and ${_vcd_actual} are not equal.")
  endif()
elseif(FI_EXPECT_REGEX_MATCH)
  string(REGEX MATCH ${FI_EXPECT_REGEX_MATCH} _mismatch_pos "${_combined}")
  if(NOT _mismatch_pos)
	  message(FATAL_ERROR "Simulation was expected to report ${FI_EXPECT_REGEX_MATCH}, but did not. See ${FI_LOG}. Result: ${_mismatch_pos}")
  endif()
endif()
