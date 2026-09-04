# SPDX-License-Identifier: Apache-2.0

file(GLOB _campaigns LIST_DIRECTORIES false "${FI_CAMPAIGN_DIR}/*")
list(SORT _campaigns)
list(LENGTH _campaigns _campaign_count)
if(_campaign_count EQUAL 0)
  message(FATAL_ERROR "No campaign files found in ${FI_CAMPAIGN_DIR}")
endif()

file(MAKE_DIRECTORY "${FI_LOG_DIR}")
set(_failures 0)
set(_failure_details)
set(_index 0)

foreach(_campaign IN LISTS _campaigns)
  math(EXPR _index "${_index} + 1")
  set(_mdir "${FI_LOG_DIR}/verilator-run-${_index}")
  set(_log "${FI_LOG_DIR}/run-${_index}.log")
  set(_vcd "${FI_LOG_DIR}/run-${_index}.vcd")
  file(REMOVE_RECURSE "${_mdir}")
  file(REMOVE "${_vcd}")
  file(MAKE_DIRECTORY "${_mdir}")

  execute_process(
    COMMAND
      "${VERILATOR_EXECUTABLE}"
      --binary --vpi --Mdir "${_mdir}"
      --trace --trace-vcd
      -CFLAGS -g
      -j "${FI_JOBS}"
      "${FI_PUBLIC_FLAT_ARG}"
      ${FI_VERILATOR_SOURCES}
      "${FI_FAULT_INJECTOR_SV}"
      -LDFLAGS "-L${FI_FAULT_INJECTOR_LIB_DIR} -lfaultergeist-inject"
      -DFAULT_INJECTION_ENABLE
      "-DFAULT_INJECTION_CAMPAIGN_FILE=\"${_campaign}\""
      "-DVCD_OUTPUT_PATH=\"${_vcd}\""
    WORKING_DIRECTORY "${FI_WORK_DIR}"
    RESULT_VARIABLE _verilator_result
    OUTPUT_VARIABLE _verilator_stdout
    ERROR_VARIABLE _verilator_stderr
  )
  if(NOT _verilator_result EQUAL 0)
    file(WRITE "${_log}" "${_verilator_stdout}${_verilator_stderr}")
    math(EXPR _failures "${_failures} + 1")
    list(APPEND _failure_details "${_index}: verilator failed for ${_campaign}. See ${_log}")
    continue()
  endif()

  execute_process(
    COMMAND "${_mdir}/Vtop" +trace
    WORKING_DIRECTORY "${FI_WORK_DIR}"
    RESULT_VARIABLE _sim_result
    OUTPUT_VARIABLE _sim_stdout
    ERROR_VARIABLE _sim_stderr
  )
  set(_combined "${_sim_stdout}${_sim_stderr}")
  file(WRITE "${_log}" "${_combined}")

  if(NOT _sim_result EQUAL 0)
    math(EXPR _failures "${_failures} + 1")
    list(APPEND _failure_details "${_index}: simulation exited with ${_sim_result} for ${_campaign}. See ${_log}")
  else()
    string(FIND "${_combined}" "Mismatch" _mismatch_pos)
    if(_mismatch_pos EQUAL -1)
      math(EXPR _failures "${_failures} + 1")
      list(APPEND _failure_details "${_index}: expected Mismatch was not reported for '${_campaign}'. See '${_log}'")
    endif()
  endif()
endforeach()

if(_failures GREATER 0)
  list(JOIN _failure_details "\n  " _failure_summary)
  math(EXPR _successes "${_campaign_count} - ${_failures}")
  message(FATAL_ERROR "${_successes}/${_campaign_count} campaigns succeeded:\n  ${_failure_summary}")
else()
  file(WRITE "${FI_LOG_DIR}/summary.log" "Result: ${_campaign_count}/${_campaign_count}\n")
endif()
