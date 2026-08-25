# SPDX-License-Identifier: Apache-2.0

# Prepares the checked-out VeeR submodule using .veer-setup from .ci.yml.

if(NOT FI_PROJECT_SOURCE_DIR)
  message(FATAL_ERROR "FI_PROJECT_SOURCE_DIR is required")
endif()
set(VEER_ROOT "${FI_PROJECT_SOURCE_DIR}/third_party/Cores-VeeR-EL2")
set(VEER_PATCH "${FI_PROJECT_SOURCE_DIR}/test/veer/veer_synth_and_sim_prep.patch")
if(NOT EXISTS "${VEER_ROOT}/.git")
  message(FATAL_ERROR
    "VeeR submodule is not downloaded. Run: git submodule update --init third_party/Cores-VeeR-EL2")
endif()

find_program(GIT_EXECUTABLE git)
find_program(PYTHON_EXECUTABLE python3)
if(NOT GIT_EXECUTABLE OR NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "VeeR setup requires git and python3")
endif()

#==============================================================================
function(run)
  execute_process(
    COMMAND ${ARGN}
    WORKING_DIRECTORY "${VEER_ROOT}"
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    string(JOIN " " command ${ARGN})
    message(FATAL_ERROR "Command failed: ${command}")
  endif()
endfunction()
macro(require_force action)
  if(NOT FI_E2E_FORCE_SETUP_VEER)
    message(FATAL_ERROR
      "${action} is required. Reconfigure with -DFI_E2E_FORCE_SETUP_VEER=ON or apply preparation manually")
  endif()
endmacro()
#==============================================================================

# Initialize missing or mismatched nested submodules.
execute_process(
  COMMAND "${GIT_EXECUTABLE}" submodule status --recursive
  WORKING_DIRECTORY "${VEER_ROOT}"
  RESULT_VARIABLE submodule_check
  OUTPUT_VARIABLE submodule_status
)
if(NOT submodule_check EQUAL 0)
  message(FATAL_ERROR "Could not check VeeR nested submodules")
endif()
if(submodule_status MATCHES "(^|\n)[-+]")
  require_force("VeeR nested submodule update")
  run("${GIT_EXECUTABLE}" submodule update --init --recursive .)
endif()

# Apply the VeeR preparation patch once.
execute_process(
  COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${VEER_PATCH}"
  WORKING_DIRECTORY "${VEER_ROOT}"
  RESULT_VARIABLE patch_applied
  OUTPUT_QUIET ERROR_QUIET
)
if(NOT patch_applied EQUAL 0)
  require_force("VeeR preparation patch")
  run("${GIT_EXECUTABLE}" apply "${VEER_PATCH}")
endif()

# Create the VeeR virtualenv when it is missing.
if(NOT EXISTS "${VEER_ROOT}/.venv/bin/python")
  require_force("VeeR virtualenv creation")
  run("${PYTHON_EXECUTABLE}" -m venv "${VEER_ROOT}/.venv")
endif()

# Resolve requirements without changing the virtualenv.
execute_process(
  COMMAND "${VEER_ROOT}/.venv/bin/python" -m pip install
          --dry-run --quiet --report - -r "${VEER_ROOT}/requirements.txt"
  WORKING_DIRECTORY "${VEER_ROOT}"
  RESULT_VARIABLE requirements_check
  OUTPUT_VARIABLE requirements_report
)
if(NOT requirements_check EQUAL 0)
  message(FATAL_ERROR "Could not check VeeR Python requirements")
endif()
if(NOT requirements_report MATCHES "\"install\"[ \t\r\n]*:[ \t\r\n]*\\[[ \t\r\n]*\\]")
  require_force("VeeR Python requirements installation")
  run("${VEER_ROOT}/.venv/bin/python" -m pip install --quiet
      -r "${VEER_ROOT}/requirements.txt")
endif()

# Generate the dual-core lockstep snapshot inside VEER_ROOT.
run("${CMAKE_COMMAND}" -E env
    "PATH=${VEER_ROOT}/.venv/bin:$ENV{PATH}"
    "PWD=${VEER_ROOT}"
    "RV_ROOT=${VEER_ROOT}"
    "${VEER_ROOT}/configs/veer.config"
    -set=lockstep_enable=1
    -set=lockstep_delay=2
    -set=lockstep_regfile_enable=0
    -snapshot=dual)
