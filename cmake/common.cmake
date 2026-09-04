# SPDX-License-Identifier: Apache-2.0

include(ProcessorCount)

set(FI_E2E_SCRIPT_DIR "${PROJECT_SOURCE_DIR}/test/cmake")
set(FI_E2E_SYNTH_SCRIPT "${PROJECT_SOURCE_DIR}/test/e2e/synth.tcl")
set(FI_E2E_FAULT_INJECTOR_DIR "${PROJECT_SOURCE_DIR}/src/FaultInjector")
set(FI_E2E_TOOLS_TARGET fi-e2e-tools)
set(FI_E2E_VEER_SETUP_TARGET fi-e2e-veer-setup)
set(FI_E2E_VEER_COMMON_DEFINES "${PROJECT_SOURCE_DIR}/third_party/Cores-VeeR-EL2/snapshots/dual/common_defines.vh")
set(FI_E2E_SKIP_CODE 77)

# Cache variables
set(ASAP7_LIBERTY_CACHE_PATH "" CACHE FILEPATH "Existing decompressed ASAP7 Liberty file to reuse for e2e tests")
option(FI_E2E_FORCE_SETUP_VEER "Run VeeR setup steps when building VeeR E2E targets" OFF)

# Default file names
set(FI_E2E_JSON_NETLIST_DEFAULT_FILENAME "netlist.json")
set(FI_E2E_VERILOG_NETLIST_DEFAULT_FILENAME "netlist.v")
set(FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME "obj_dir")
set(FI_E2E_FAULTGEN_CONFIG_DEFAULT_FILENAME "config.json")
set(FI_E2E_FAULT_CAMPAIGN_OUT_DEFAULT_FILENAME "fault_campaign_out.csv")
set(FI_E2E_VLT_CONFIG_DEFAULT_FILENAME "flat_signals.vlt")
set(FI_E2E_TB_TOP_DEFAULT_FILENAME "Vtop")
set(FI_E2E_CAMPAIGN_DIR_DEFAULT_FILENAME "fault_campaign")
set(FI_E2E_VCD_OUTPUT_PATH_DEFAULT_FILENAME "vlt_dump.vcd")


# pdk related variables
set(FI_E2E_ASAP7_ROOT "${PROJECT_SOURCE_DIR}/third_party/asap7")
list(APPEND FI_E2E_ASAP7_LIBERTY_FILENAMES "asap7sc7p5t_SEQ_RVT_TT_nldm_220123.lib"
                                   "asap7sc7p5t_SIMPLE_RVT_TT_nldm_211120.lib"
                                   "asap7sc7p5t_INVBUF_RVT_TT_nldm_220122.lib"
                                   "asap7sc7p5t_OA_RVT_TT_nldm_211120.lib"
                                   "asap7sc7p5t_AO_RVT_TT_nldm_211120.lib")

set(FI_E2E_ASAP7_LIBERTY_ARCHIVES "")
foreach(lib_file IN LISTS FI_E2E_ASAP7_LIBERTY_FILENAMES)
    list(APPEND FI_E2E_ASAP7_LIBERTY_ARCHIVES "${FI_E2E_ASAP7_ROOT}/LIB/NLDM/${lib_file}.7z")
endforeach()
foreach(lib_file IN LISTS FI_E2E_ASAP7_LIBERTY_FILENAMES)
    list(APPEND FI_E2E_LIB_FILES "${CMAKE_BINARY_DIR}/test/pdk/${lib_file}")
endforeach()
set(FI_E2E_PDK_TARGET fi_e2e_pdk)

# Argument/default conventions used by the helpers below:
#
# - Parsed keyword arguments use the FI_ prefix produced by
#   cmake_parse_arguments(). For example, OUTPUT becomes FI_OUTPUT.
# - _fi_default_arg(FI_ARG ARG) resolves scalar arguments in this order:
#     1. Explicit parsed argument, e.g. FI_ARG.
#     2. Variable in the caller directory/function scope, e.g. ARG.
#     3. Directory property definition for ARG.
#   It errors if none of those sources provides a non-empty value.
# - _fi_resolve_implicit_arg_with_default(NAME DEFAULT_FILENAME FALLBACK_DIRECTORY_ARG_NAME) resolves path
#   arguments in this order:
#     1. Explicit parsed argument FI_${NAME}.
#     2. Variable ${NAME}.
#     3. Directory property definition for ${NAME}.
#     4. ${FALLBACK_DIRECTORY_ARG_NAME}/${DEFAULT_FILENAME}, if fallback exists.
#     5. DEFAULT_FILENAME, if no fallback directory is supplied.
#
# Common variables expected by most tests:
#   WORK_DIR         Build-tree working root for the scenario.
#   SIMULATION_DIR  Per-run output directory. Defaults to ${WORK_DIR}/obj_dir.
#   SOURCES         Verilator source list.
#   RTL_ROOT        Source fixture directory exported to Yosys scripts.
#   DESIGN_TOP      Top design module exported to Yosys scripts/configs.
#   DESIGN_FILE_LIST
#                   Yosys input file list.
#   JSON_NETLIST    Yosys JSON netlist output.
#   LIB_FILES       Liberty files used by FaultGenerationTool configs.
#   VCD_OUTPUT_PATH Path passed to Verilator for $dumpfile.
#   FAULT_CAMPAIGN_OUT
#                   faultergeist-gen output. Defaults to
#                   ${SIMULATION_DIR}/fault_campaign_out.csv.


# after _fi_default_arg variable named "${ARG_NAME}" will be holding the value
# it is supposed to, or configuration will fail
# 1. It starts by checking if it is already populated
# 2. Uses value from fallback variable (if it is defined and populated)
# 3. Uses directory_property(${ARG_NAME}) if available
# 4. Fails
macro(_fi_default_arg ARG_NAME FALLBACK_NAME)
  set(_fi_default_value "")
  if(DEFINED ${ARG_NAME} AND NOT "${${ARG_NAME}}" STREQUAL "")
  elseif(DEFINED ${FALLBACK_NAME} AND NOT "${${FALLBACK_NAME}}" STREQUAL "")
    set(${ARG_NAME} "${${FALLBACK_NAME}}")
  else()
    get_directory_property(_fi_default_value DEFINITION ${FALLBACK_NAME})
    if(NOT "${_fi_default_value}" STREQUAL "")
      set(${ARG_NAME} "${_fi_default_value}")
    else()
      message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} requires ${ARG_NAME} or ${FALLBACK_NAME}")
    endif()
  endif()
endmacro()

# After _fi_resolve_implicit_arg_with_default variable "FI_${ARG_NAME}" will be holding the
# value it is supposed to, or configuration will fail
# 1. It starts by checking if it (FI_${ARG_NAME}) is already populated
# 2. Uses value under ${ARG_NAME} if it is defined
# 3. Uses directory_property(${ARG_NAME}) if available
# 4.1. Saves value under ${FALLBACK_DIRECTORY_ARG_NAME} if is defined
# 4.2. Saves directory_property(${FALLBACK_DIRECTORY_ARG_NAME}) if is defined
# 5. Uses "${saved value}/${DEFAULT_FILENAME}" if step 4 saved any value and DEFAULT_FILENAME is defined
# 6. Uses DEFAULT_FILENAME if DEFAULT_FILENAME is defined
#
# Use this when some path should be possible to pass implicitly through caller scope or directory properties
macro(_fi_resolve_implicit_arg_with_default ARG_NAME DEFAULT_FILENAME FALLBACK_DIRECTORY_ARG_NAME)
  set(_fi_path_arg "FI_${ARG_NAME}")
  set(_fi_path_value "")
  set(_fi_fallback_value "")
  if(DEFINED ${_fi_path_arg} AND NOT "${${_fi_path_arg}}" STREQUAL "")
  elseif(DEFINED ${ARG_NAME} AND NOT "${${ARG_NAME}}" STREQUAL "")
    set(${_fi_path_arg} "${${ARG_NAME}}")
  else()
    get_directory_property(_fi_path_value DEFINITION ${ARG_NAME})
    if(NOT "${_fi_path_value}" STREQUAL "")
      set(${_fi_path_arg} "${_fi_path_value}")
    else()
      if(NOT "${FALLBACK_DIRECTORY_ARG_NAME}" STREQUAL "")
        if(DEFINED ${FALLBACK_DIRECTORY_ARG_NAME} AND NOT "${${FALLBACK_DIRECTORY_ARG_NAME}}" STREQUAL "")
          set(_fi_fallback_value "${${FALLBACK_DIRECTORY_ARG_NAME}}")
        else()
          get_directory_property(_fi_fallback_value DEFINITION ${FALLBACK_DIRECTORY_ARG_NAME})
        endif()
      endif()

      if(NOT "${_fi_fallback_value}" STREQUAL "" AND NOT "${DEFAULT_FILENAME}" STREQUAL "")
        set(${_fi_path_arg} "${_fi_fallback_value}/${DEFAULT_FILENAME}")
      elseif(NOT "${DEFAULT_FILENAME}" STREQUAL "")
        set(${_fi_path_arg} "${DEFAULT_FILENAME}")
      else()
        message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION} requires ${_fi_path_arg}, ${ARG_NAME}, or ${FALLBACK_DIRECTORY_ARG_NAME}")
      endif()
    endif()
  endif()
endmacro()

macro(_fi_resolve_pdk_path_and_append_dependency DEPENDS_VAR)
  foreach(lib_file IN LISTS FI_E2E_LIB_FILES)
    message(VERBOSE "Adding ${lib_file} to command dependencies")
    list(APPEND ${DEPENDS_VAR} "${lib_file}")
  endforeach()
  list(APPEND ${DEPENDS_VAR} ${FI_E2E_PDK_TARGET})
endmacro()

macro(_fi_mark_deprecated ARG)
  if(DEFINED ${ARG})
    message(FATAL_ERROR "Argument ${ARG} is deprecated. Don't use it in ${CMAKE_CURRENT_FUNCTION}.")
  endif()
endmacro()

#===============================================================================
# Interface functions
#===============================================================================

function(fi_require_e2e_tools)
  if(NOT TARGET ${FI_E2E_VEER_SETUP_TARGET})
    add_custom_command(
      OUTPUT "${FI_E2E_VEER_COMMON_DEFINES}"
      COMMAND "${CMAKE_COMMAND}" "-DFI_PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
              "-DFI_E2E_FORCE_SETUP_VEER=${FI_E2E_FORCE_SETUP_VEER}"
              -P "${PROJECT_SOURCE_DIR}/cmake/setup_veer.cmake"
      DEPENDS "${PROJECT_SOURCE_DIR}/cmake/setup_veer.cmake"
              "${PROJECT_SOURCE_DIR}/test/veer/veer_synth_and_sim_prep.patch"
      COMMENT "Preparing VeeR dual snapshot"
      VERBATIM
    )
    add_custom_target(${FI_E2E_VEER_SETUP_TARGET}
      DEPENDS "${FI_E2E_VEER_COMMON_DEFINES}"
    )
  endif()
  message(STATUS "E2E VeeR setup: deferred to target ${FI_E2E_VEER_SETUP_TARGET}")
  message(STATUS "E2E VeeR setup enabled: ${FI_E2E_FORCE_SETUP_VEER}")
  set(_veer_root "${PROJECT_SOURCE_DIR}/third_party/Cores-VeeR-EL2")
  if(EXISTS "${_veer_root}/.git")
    message(STATUS "E2E VeeR checkout: ${_veer_root}")
  else()
    message(STATUS "E2E VeeR checkout: NOT DOWNLOADED")
  endif()
  if(EXISTS "${_veer_root}/.venv/bin/python")
    message(STATUS "E2E VeeR virtualenv: ${_veer_root}/.venv")
  else()
    message(STATUS "E2E VeeR virtualenv: pending setup")
  endif()
  if(EXISTS "${FI_E2E_VEER_COMMON_DEFINES}")
    message(STATUS "E2E VeeR dual snapshot: ready")
  else()
    message(STATUS "E2E VeeR dual snapshot: pending setup")
  endif()

  if(DEFINED VERILATOR_ROOT AND NOT VERILATOR_ROOT STREQUAL "")
    set(_verilator "${VERILATOR_ROOT}/bin/verilator")
    set(VERILATOR_EXECUTABLE "${_verilator}" CACHE FILEPATH "Path to verilator" FORCE)
  elseif(DEFINED ENV{VERILATOR_ROOT} AND NOT "$ENV{VERILATOR_ROOT}" STREQUAL "")
    set(_verilator "$ENV{VERILATOR_ROOT}/bin/verilator")
    set(VERILATOR_EXECUTABLE "${_verilator}" CACHE FILEPATH "Path to verilator" FORCE)
  else()
    find_program(VERILATOR_EXECUTABLE verilator)
  endif()
  if(VERILATOR_EXECUTABLE AND NOT "${VERILATOR_EXECUTABLE}" MATCHES "-NOTFOUND$")
    message(STATUS "E2E Verilator: ${VERILATOR_EXECUTABLE}")
  else()
    message(STATUS "E2E Verilator: NOT FOUND")
  endif()

  find_program(YOSYS_EXECUTABLE yosys)
  if(YOSYS_EXECUTABLE)
    message(STATUS "E2E Yosys: ${YOSYS_EXECUTABLE}")
  else()
    message(STATUS "E2E Yosys: NOT FOUND")
  endif()

  find_program(SEVENZIP_EXECUTABLE NAMES 7z 7za 7zr)
  if(SEVENZIP_EXECUTABLE)
    message(STATUS "E2E 7-Zip: ${SEVENZIP_EXECUTABLE}")
  else()
    message(STATUS "E2E 7-Zip: NOT FOUND")
  endif()

  ProcessorCount(_processor_count)
  if(_processor_count EQUAL 0)
    set(_processor_count 1)
  endif()
  set(FI_E2E_JOBS "${_processor_count}" CACHE STRING "Parallel jobs used by E2E Verilator/faultergeist-gen commands")
  message(STATUS "E2E parallel jobs: ${FI_E2E_JOBS}")

  set(FI_E2E_TOOLS_FOUND TRUE)
  foreach(_tool VERILATOR_EXECUTABLE YOSYS_EXECUTABLE)
    if(NOT ${_tool} OR "${${_tool}}" MATCHES "-NOTFOUND$")
      set(FI_E2E_TOOLS_FOUND FALSE)
    endif()
  endforeach()
  if(ASAP7_LIBERTY_CACHE_PATH)
    get_filename_component(_asap7_liberty_cache_path "${ASAP7_LIBERTY_CACHE_PATH}" ABSOLUTE)
    set(ASAP7_LIBERTY_CACHE_PATH "${_asap7_liberty_cache_path}" CACHE FILEPATH "Existing decompressed ASAP7 Liberty file to reuse for e2e tests" FORCE)
    if(NOT EXISTS "${ASAP7_LIBERTY_CACHE_PATH}")
      message(WARNING "ASAP7_LIBERTY_CACHE_PATH does not exist: ${ASAP7_LIBERTY_CACHE_PATH}")
      set(FI_E2E_TOOLS_FOUND FALSE)
    else()
      message(STATUS "E2E ASAP7 Liberty: ${ASAP7_LIBERTY_CACHE_PATH}")
    endif()
  else()
    set(_asap7_archives_found TRUE)
    foreach(lib_archive IN LISTS FI_E2E_ASAP7_LIBERTY_ARCHIVES)
      if(NOT EXISTS "${lib_archive}")
        message(WARNING "ASAP7 Liberty archive ${lib_archive} not found. Clone third_party/asap7 or set ASAP7_LIBERTY_CACHE_PATH.")
        set(_asap7_archives_found FALSE)
        set(FI_E2E_TOOLS_FOUND FALSE)
      endif()
    endforeach()
    if(_asap7_archives_found)
      message(STATUS "E2E ASAP7 Liberty: bundled archives ready")
    else()
      message(STATUS "E2E ASAP7 Liberty: NOT READY")
    endif()
    if(NOT SEVENZIP_EXECUTABLE OR "${SEVENZIP_EXECUTABLE}" MATCHES "-NOTFOUND$")
      message(WARNING "7z was not found. Install 7z or set ASAP7_LIBERTY_CACHE_PATH to a decompressed .lib file.")
      set(FI_E2E_TOOLS_FOUND FALSE)
    endif()
  endif()

  if(NOT FI_E2E_TOOLS_FOUND)
    message(WARNING "E2E tools not found. E2E tests will be skipped.")

    if(NOT TARGET ${FI_E2E_TOOLS_TARGET})
      add_custom_target(${FI_E2E_TOOLS_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Error: E2E tools were not found during configuration."
        COMMAND ${CMAKE_COMMAND} -E false
      )
    endif()
  else()
    message(STATUS "E2E tools: ready")
    if(NOT TARGET ${FI_E2E_TOOLS_TARGET})
        add_custom_target(${FI_E2E_TOOLS_TARGET})
    endif()
  endif()
  set(FI_E2E_TOOLS_FOUND "${FI_E2E_TOOLS_FOUND}" PARENT_SCOPE)
endfunction()

function(fi_add_ctest_scenario NAME TARGET_NAME)
  set(options)
  set(one_value_args)
  set(multi_value_args DEPENDS)
  cmake_parse_arguments("FI" "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_custom_target(${TARGET_NAME}
    DEPENDS ${FI_DEPENDS}
  )

  add_test(
    NAME "${NAME}"
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target "${TARGET_NAME}" --config $<CONFIG>
  )
  set_tests_properties("${NAME}" PROPERTIES
      LABELS "e2e;yosys;verilator"
      SKIP_RETURN_CODE ${FI_E2E_SKIP_CODE}
      TIMEOUT 3600
  )
endfunction()

# fi_add_file_comparison(<target> <first-file> <second-file>
#   [SORT] [DEPENDS <deps>...])
#
# Adds a target which fails when the two files differ. With SORT, lines are
# sorted before comparison; duplicate lines remain significant.
function(fi_add_file_comparison NAME FIRST_FILE SECOND_FILE)
  set(options SORT)
  set(multi_value_args DEPENDS)
  cmake_parse_arguments(FI "${options}" "" "${multi_value_args}" ${ARGN})

  add_custom_target("${NAME}"
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${FIRST_FILE}" "${SECOND_FILE}"
    DEPENDS "${FIRST_FILE}" "${SECOND_FILE}" ${FI_DEPENDS}
    VERBATIM
  )
endfunction()

# fi_add_yosys_json(<target>
#   [SCRIPT <path>] [WORK_DIR <dir>] [RTL_ROOT <dir>]
#   [DESIGN_FILE_LIST <file list>] [DESIGN_TOP <module>]
#   [SYNTH_HDL_FRONTEND <frontend>] [JSON_NETLIST <path>]
#   [VERILOG_NETLIST <path>] [LIB_FILES <liberty>...] [ENV <VAR=value>...])
#
# Required, explicit or implicit:
#   SCRIPT             Yosys Tcl script. Defaults to ${FI_E2E_SYNTH_SCRIPT}.
#                      May come from SCRIPT.
#   WORK_DIR           Build working directory.
#   DESIGN_FILE_LIST   Yosys input source. Specifies RTL sources. May come from DESIGN_FILE_LIST.
#   RTL_ROOT           Fixture directory exported to the script. May come from
#                      RTL_ROOT.
#   DESIGN_TOP         Top design module exported to the script. May come from DESIGN_TOP.
#
# Deprecated:
#   OUTPUT             Use JSON_NETLIST instead. Passing OUTPUT is an error.
#
# Optional/implicit:
#   JSON_NETLIST       Defaults to ${WORK_DIR}/netlist.json.
#   VERILOG_NETLIST    Defaults to ${WORK_DIR}/netlist.v.
#   SYNTH_HDL_FRONTEND Exported when supplied or set in caller scope.
#   LIB_FILES          list of paths to liberty files used for techmapping
#
# Build-time environment passed to Yosys:
#   JSON_NETLIST=<JSON_NETLIST>
#   VERILOG_NETLIST=<VERILOG_NETLIST>
#   RTL_ROOT=<RTL_ROOT>
#   DESIGN_FILE_LIST=<DESIGN_FILE_LIST>
#   DESIGN_TOP=<DESIGN_TOP>
#   SYNTH_HDL_FRONTEND=<SYNTH_HDL_FRONTEND>
#   plus any values from ENV.
function(fi_add_yosys_json NAME)
  set(options NO_TECHMAP)
  set(one_value_args SCRIPT OUTPUT WORK_DIR RTL_ROOT DESIGN_FILE_LIST DESIGN_TOP SYNTH_HDL_FRONTEND JSON_NETLIST VERILOG_NETLIST)
  set(multi_value_args ENV LIB_FILES)
  cmake_parse_arguments("FI" "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_mark_deprecated(FI_OUTPUT)

  _fi_resolve_implicit_arg_with_default(SCRIPT "${FI_E2E_SYNTH_SCRIPT}" "")
  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_default_arg(FI_RTL_ROOT RTL_ROOT)
  _fi_default_arg(FI_DESIGN_FILE_LIST DESIGN_FILE_LIST)
  _fi_default_arg(FI_DESIGN_TOP DESIGN_TOP)
  if(NOT FI_SYNTH_HDL_FRONTEND AND DEFINED SYNTH_HDL_FRONTEND)
    set(FI_SYNTH_HDL_FRONTEND "${SYNTH_HDL_FRONTEND}")
  endif()

  _fi_resolve_implicit_arg_with_default(JSON_NETLIST "${FI_E2E_JSON_NETLIST_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(VERILOG_NETLIST "${FI_E2E_VERILOG_NETLIST_DEFAULT_FILENAME}" WORK_DIR)

  set(_log_file "${FI_WORK_DIR}/${FI_DESIGN_TOP}_yosys.log")

  get_filename_component(_output_dir "${FI_JSON_NETLIST}" DIRECTORY)
  set(_depends "${FI_SCRIPT}" ${FI_DESIGN_FILE_LIST})


  string(JOIN "\" \"" LIB_FILES_STR ${FI_E2E_LIB_FILES})
  set(LIB_FILES_ARG "LIB_FILES=\"${LIB_FILES_STR}\"")
  if (FI_NO_TECHMAP)
    set(LIB_FILES_ARG "")
  else()
    _fi_resolve_pdk_path_and_append_dependency(_depends)
  endif()

  add_custom_command(
    OUTPUT "${FI_JSON_NETLIST}" "${FI_VERILOG_NETLIST}" "${_log_file}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}" "${FI_WORK_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E env
    "JSON_NETLIST=${FI_JSON_NETLIST}"
    "VERILOG_NETLIST=${FI_VERILOG_NETLIST}"
    "DESIGN_FILE_LIST=${FI_DESIGN_FILE_LIST}"
    "DESIGN_TOP=${FI_DESIGN_TOP}"
    "RTL_ROOT=${FI_RTL_ROOT}"
    ${LIB_FILES_ARG}
    "SYNTH_HDL_FRONTEND=${FI_SYNTH_HDL_FRONTEND}"
      ${FI_ENV}
    "${YOSYS_EXECUTABLE}" -q -t -d -l "${_log_file}" -c "${FI_SCRIPT}"
    DEPENDS ${_depends}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${FI_JSON_NETLIST}" "${FI_VERILOG_NETLIST}" "${_log_file}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()

# fi_configure_file(<input> [OUTPUT <path>] [SIMULATION_DIR <dir>]
#                   [DEPENDS <files>...])
#
# Configures <input> at build time with @ONLY substitution on forwarded variables.
#
# Required:
#   input              Template file.
#
# Optional/implicit:
#   OUTPUT            Configured output file. Defaults to
#                     ${SIMULATION_DIR}/config.json.
#   SIMULATION_DIR    May be explicit, come from SIMULATION_DIR, or default to
#                     ${WORK_DIR}/obj_dir.
#   Forwarded variables:
#                     DESIGN_TOP, JSON_NETLIST, NETLIST_PATH,
#                     FAULT_CAMPAIGN_OUT, CAMPAIGN_DIR, FI_E2E_JOBS.
function(fi_configure_file INPUT)
  set(options)
  set(one_value_args OUTPUT SIMULATION_DIR)
  set(multi_value_args EXT_LIB_FILES DEPENDS)
  cmake_parse_arguments(FI "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_implicit_arg_with_default(SIMULATION_DIR "${FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(OUTPUT "${FI_E2E_FAULTGEN_CONFIG_DEFAULT_FILENAME}" FI_SIMULATION_DIR)
  _fi_resolve_implicit_arg_with_default(JSON_NETLIST "${FI_E2E_JSON_NETLIST_DEFAULT_FILENAME}" WORK_DIR)
  _fi_default_arg(FI_EXT_LIB_FILES LIB_FILES)


  set(_fi_configure_defs)
  # Variables forwarded to configure_file_at_build.cmake for @ONLY substitution.
  foreach(_fi_var DESIGN_TOP JSON_NETLIST VERILOG_NETLIST NETLIST_PATH FAULT_CAMPAIGN_OUT CAMPAIGN_DIR FI_E2E_JOBS)
    set(_fi_arg_var "FI_${_fi_var}")
    if(DEFINED ${_fi_arg_var} AND NOT "${${_fi_arg_var}}" STREQUAL "")
      list(APPEND _fi_configure_defs "-D${_fi_var}=${${_fi_arg_var}}")
    elseif(DEFINED ${_fi_var})
      list(APPEND _fi_configure_defs "-D${_fi_var}=${${_fi_var}}")
    else()
      get_directory_property(_fi_value DEFINITION ${_fi_var})
      if(NOT "${_fi_value}" STREQUAL "")
        list(APPEND _fi_configure_defs "-D${_fi_var}=${_fi_value}")
      endif()
    endif()
  endforeach()

  set(_depends "${INPUT}" "${FI_E2E_SCRIPT_DIR}/configure_file_at_build.cmake" ${FI_DEPENDS})

  get_filename_component(_output_dir "${FI_OUTPUT}" DIRECTORY)

  string(JOIN "\", \"" LIB_FILES_STR ${FI_EXT_LIB_FILES})
  add_custom_command(
    OUTPUT "${FI_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DFI_INPUT=${INPUT}"
      "-DFI_OUTPUT=${FI_OUTPUT}"
      ${_fi_configure_defs}
      -DLIB_FILES="${LIB_FILES_STR}"
      -P "${FI_E2E_SCRIPT_DIR}/configure_file_at_build.cmake"
    DEPENDS ${_depends}
    VERBATIM
  )
endfunction()

# fi_add_fault_campaign(<target>
#   [ALLOW_EMPTY] [OUTPUT <path>] [CONFIG <path> | CONFIG_FILE <path>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [ARGS <args...>] [DEPENDS <deps...>])
#
# Generates a fault campaign with faultergeist-gen.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Per-run output directory. Defaults to ${WORK_DIR}/obj_dir.
#
# Optional/implicit:
#   OUTPUT            CMake-tracked output. Defaults to FAULT_CAMPAIGN_OUT.
#   FAULT_CAMPAIGN_OUT
#                     Tool output path. Defaults to
#                     ${SIMULATION_DIR}/fault_campaign_out.csv.
#   CONFIG_FILE
#                     Config file passed as --config_file. If neither CONFIG_FILE
#                     nor ARGS is supplied, defaults to
#                     ${SIMULATION_DIR}/config.json.
#   ARGS              Direct faultergeist-gen arguments. When ARGS is
#                     non-empty, no implicit config file is added.
#   ALLOW_EMPTY       Do not fail when the generated campaign is empty.
#
# OUTPUT may be a stamp file. When OUTPUT differs from FAULT_CAMPAIGN_OUT, the
# campaign path is registered as a byproduct.
#
# On rerun, the helper removes OUTPUT and FAULT_CAMPAIGN_OUT, recreates the
# required directories, invokes the tool, then touches OUTPUT.
function(fi_add_fault_campaign NAME)
  set(options ALLOW_EMPTY NO_VLT_CONFIG EXTRA_DEBUG)
  set(one_value_args OUTPUT CONFIG_FILE WORK_DIR SIMULATION_DIR)
  set(multi_value_args ARGS DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_implicit_arg_with_default(SIMULATION_DIR "${FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(FAULT_CAMPAIGN_OUT "${FI_E2E_FAULT_CAMPAIGN_OUT_DEFAULT_FILENAME}" SIMULATION_DIR)

  if(NOT FI_OUTPUT)
    set(FI_OUTPUT "${FI_FAULT_CAMPAIGN_OUT}")
  endif()

  if(FI_CONFIG_FILE)
    set(FI_CONFIG "${FI_CONFIG_FILE}")
  elseif(NOT FI_CONFIG AND NOT FI_ARGS)
    set(FI_CONFIG "${FI_SIMULATION_DIR}/config.json")
  endif()

  if(NOT FI_WORK_DIR)
    _fi_default_arg(FI_WORK_DIR WORK_DIR)
  endif()

  set(_command "$<TARGET_FILE:faultergeist-gen>" ${FI_ARGS})
  set(_log_file "${FI_OUTPUT}_gen.log")
  if(FI_CONFIG)
    list(APPEND _command "--config_file" "${FI_CONFIG}")
  endif()
  if(NOT FI_NO_VLT_CONFIG)
    _fi_resolve_implicit_arg_with_default(VLT_CONFIG "${FI_E2E_VLT_CONFIG_DEFAULT_FILENAME}" SIMULATION_DIR)
    list(APPEND _command "--vlt_config" "${FI_VLT_CONFIG}")
  endif()
  if(FI_EXTRA_DEBUG)
    list(APPEND _command --v=3 --stderrthreshold=0)
  endif()

  get_filename_component(_output_dir "${FI_OUTPUT}" DIRECTORY)
  get_filename_component(_fault_campaign_out_dir "${FI_FAULT_CAMPAIGN_OUT}" DIRECTORY)
  set(_make_dirs "${FI_WORK_DIR}")
  if(NOT "${_output_dir}" STREQUAL "")
    list(APPEND _make_dirs "${_output_dir}")
  endif()
  if(NOT "${_fault_campaign_out_dir}" STREQUAL "")
    list(APPEND _make_dirs "${_fault_campaign_out_dir}")
  endif()
  set(_byproducts)
  if(NOT "${FI_OUTPUT}" STREQUAL "${FI_FAULT_CAMPAIGN_OUT}")
    set(_byproducts BYPRODUCTS "${FI_FAULT_CAMPAIGN_OUT}")
  endif()
  set(_outputs "${FI_OUTPUT}")
  set(_remove_outputs "${FI_OUTPUT}" "${FI_FAULT_CAMPAIGN_OUT}")
  if(FI_VLT_CONFIG)
    list(APPEND _outputs "${FI_VLT_CONFIG}")
    list(APPEND _remove_outputs "${FI_VLT_CONFIG}")
  endif()

  set(_depends faultergeist-gen ${FI_CONFIG} ${FI_DEPENDS})
  _fi_resolve_pdk_path_and_append_dependency(_depends)

  set(_check_campaign)
  if(NOT FI_ALLOW_EMPTY)
    set(_check_campaign
      COMMAND "${CMAKE_COMMAND}"
        "-DFI_FAULT_CAMPAIGN=${FI_FAULT_CAMPAIGN_OUT}"
        -P "${FI_E2E_SCRIPT_DIR}/check_fault_campaign.cmake"
    )
  endif()

  add_custom_command(
    OUTPUT ${_outputs}
    ${_byproducts}
    COMMAND "${CMAKE_COMMAND}" -E rm -rf ${_remove_outputs}
    COMMAND "${CMAKE_COMMAND}" -E make_directory ${_make_dirs}
    COMMAND bash -c "\"$@\" > \"${_log_file}\" 2>&1" -- ${_command}
    ${_check_campaign}
    COMMAND "${CMAKE_COMMAND}" -E touch "${FI_OUTPUT}"
    DEPENDS ${_depends}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${FI_OUTPUT}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()

# fi_add_verilated_sim(<target>
#   [FAULT_INJECTION] [TRACE] [NO_MAIN] [NO_BUILD] [VEER_MODE]
#   [CAMPAIGN_FILE <path>] [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [TB_TOP <name>] [VCD_OUTPUT_PATH <path>] [VLT_CONFIG <path>]
#   [SOURCES <files...>] [DEPENDS <deps...>]
#   [VERILATOR_EXTRA_ARGS <args...>])
#
# Builds a Verilator binary in ${SIMULATION_DIR}.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Verilator --Mdir and output directory. Defaults to
#                    ${WORK_DIR}/obj_dir.
#   SOURCES           Verilator input sources. May come from SOURCES.
#   VCD_OUTPUT_PATH   Path compiled into the testbench for $dumpfile. May come
#                     from VCD_OUTPUT_PATH. Defaults to vlt_dump.vcd.
#   VLT_CONFIG        Verilator signal configuration. Defaults to
#                     ${SIMULATION_DIR}/flat_signals.vlt.
#
# Common Verilator flags:
#   --vpi --Mdir <SIMULATION_DIR> --prefix <TB_TOP>
#   -CFLAGS -g
#   -j <FI_E2E_JOBS>
#
# Build mode:
#   Default uses --binary. NO_MAIN/NO_BUILD use --exe --cc --timing, optionally
#   adding --main and/or --build. VEER_MODE implies NO_BUILD and then runs make
#   manually.
#
# Fault-injection mode:
#   FAULT_INJECTION adds faultergeist-inject.sv, links libfaultergeist-inject, defines
#   FAULT_INJECTION_ENABLE, and sets FAULT_INJECTION_CAMPAIGN_FILE.
#   CAMPAIGN_FILE may be explicit; otherwise it defaults to FAULT_CAMPAIGN_OUT,
#   which defaults to ${SIMULATION_DIR}/fault_campaign_out.csv.
#   The Verilator build depends on FAULT_CAMPAIGN_OUT.
#
# Trace mode:
#   TRACE adds --trace --trace-vcd to verilator call
#
# Output:
#   ${SIMULATION_DIR}/${TB_TOP} is exposed as <target>_EXECUTABLE in parent scope.
function(fi_add_verilated_sim NAME)
  set(options FAULT_INJECTION TRACE NO_MAIN NO_BUILD VEER_MODE PUBLIC_FLAT_RW)
  set(one_value_args CAMPAIGN_FILE WORK_DIR SIMULATION_DIR TB_TOP VCD_OUTPUT_PATH VLT_CONFIG)
  set(multi_value_args SOURCES DEPENDS VERILATOR_EXTRA_ARGS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_default_arg(FI_SOURCES SOURCES)
  _fi_resolve_implicit_arg_with_default(SIMULATION_DIR "${FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(FAULT_CAMPAIGN_OUT "${FI_E2E_FAULT_CAMPAIGN_OUT_DEFAULT_FILENAME}" SIMULATION_DIR)
  _fi_resolve_implicit_arg_with_default(TB_TOP "${FI_E2E_TB_TOP_DEFAULT_FILENAME}" "")
  _fi_resolve_implicit_arg_with_default(VCD_OUTPUT_PATH "${FI_E2E_VCD_OUTPUT_PATH_DEFAULT_FILENAME}" "")
  _fi_resolve_implicit_arg_with_default(VLT_CONFIG "${FI_E2E_VLT_CONFIG_DEFAULT_FILENAME}" SIMULATION_DIR)

  set(_exe "${FI_SIMULATION_DIR}/${FI_TB_TOP}")
  set(_mdir "${FI_SIMULATION_DIR}")
  set(_args)
  set(_depends ${FI_DEPENDS})
  if(FI_FAULT_INJECTION)
    if(NOT FI_CAMPAIGN_FILE)
      _fi_resolve_implicit_arg_with_default(FAULT_CAMPAIGN_OUT fault_campaign_out.csv SIMULATION_DIR)
      set(FI_CAMPAIGN_FILE "${FI_FAULT_CAMPAIGN_OUT}")
    endif()
    list(APPEND _args "${FI_E2E_FAULT_INJECTOR_DIR}/faultergeist-inject.sv")
    list(APPEND _args -LDFLAGS "-L$<TARGET_FILE_DIR:faultergeist-inject> -lfaultergeist-inject")
    list(APPEND _args -DFAULT_INJECTION_ENABLE)
    list(APPEND _args "-DFAULT_INJECTION_CAMPAIGN_FILE=\"${FI_CAMPAIGN_FILE}\"")
    list(APPEND _depends faultergeist-inject "${FI_E2E_FAULT_INJECTOR_DIR}/faultergeist-inject.sv" "${FI_FAULT_CAMPAIGN_OUT}")
  endif()
  if(FI_TRACE)
    list(APPEND _args --trace --trace-vcd)
  endif()
  if(FI_TRACE OR FI_FAULT_INJECTION)
    if(FI_PUBLIC_FLAT_RW)
      list(APPEND _args "--public-flat-rw")
    else()
      list(APPEND _args "${FI_VLT_CONFIG}")
      list(APPEND _depends "${FI_VLT_CONFIG}")
    endif()
  endif()
  if(FI_VERILATOR_EXTRA_ARGS)
    list(APPEND _args ${FI_VERILATOR_EXTRA_ARGS})
  endif()
  list(APPEND _args "-DVCD_OUTPUT_PATH=\"${FI_VCD_OUTPUT_PATH}\"")
  list(APPEND _args -CFLAGS "-DVCD_OUTPUT_PATH=\\\"${FI_VCD_OUTPUT_PATH}\\\"")

  if(FI_VEER_MODE)
    set(FI_NO_BUILD)
    list(APPEND _depends "${FI_E2E_VEER_SETUP_TARGET}")
  endif()

  if(FI_NO_MAIN OR FI_NO_BUILD)
    set(_build_arg --exe --cc --timing)
    if(NOT FI_NO_MAIN)
      set(_build_arg ${_build_arg} --main)
    endif()
    if(NOT FI_NO_BUILD)
      set(_build_arg ${_build_arg} --build)
    endif()
  else()
    set(_build_arg --binary)
  endif()

  list(APPEND _build_arg --prefix ${FI_TB_TOP})

  # Special case for VeeR testing - custom step is required before building the simulation binary
  if(FI_VEER_MODE)
    add_custom_command(
      OUTPUT "${_exe}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${FI_SIMULATION_DIR}" "${FI_WORK_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${FI_SIMULATION_DIR}/CMakeFiles" "${FI_SIMULATION_DIR}/${FI_TB_TOP}.dir"
      COMMAND "${CMAKE_COMMAND}" -E rm -f "${FI_SIMULATION_DIR}/${FI_TB_TOP}" "${FI_SIMULATION_DIR}/${FI_TB_TOP}__ALL.a" "${FI_SIMULATION_DIR}/${FI_TB_TOP}.mk" "${FI_SIMULATION_DIR}/${FI_TB_TOP}_classes.mk"
      COMMAND
        "${CMAKE_COMMAND}" -E env "PATH=${RTL_ROOT}/.venv/bin:$ENV{PATH}" "RV_ROOT=${RTL_ROOT}"
        "${VERILATOR_EXECUTABLE}"
        ${_build_arg} --vpi --Mdir "${_mdir}"
        -CFLAGS "-g"
        -j "${FI_E2E_JOBS}"
        ${FI_SOURCES}
        ${_args}
      COMMAND cp "${RTL_ROOT}/testbench/test_tb_top.cpp" "${FI_SIMULATION_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E env "PATH=${RTL_ROOT}/.venv/bin:$ENV{PATH}" "RV_ROOT=${RTL_ROOT}" make -e -C "${FI_SIMULATION_DIR}" -f "${FI_TB_TOP}.mk"
      DEPENDS ${_depends}
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT "${_exe}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${FI_SIMULATION_DIR}" "${FI_WORK_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E rm -rf "${FI_SIMULATION_DIR}/CMakeFiles" "${FI_SIMULATION_DIR}/${FI_TB_TOP}.dir"
      COMMAND "${CMAKE_COMMAND}" -E rm -f "${FI_SIMULATION_DIR}/${FI_TB_TOP}" "${FI_SIMULATION_DIR}/${FI_TB_TOP}__ALL.a" "${FI_SIMULATION_DIR}/${FI_TB_TOP}.mk" "${FI_SIMULATION_DIR}/${FI_TB_TOP}_classes.mk"
      COMMAND
        "${VERILATOR_EXECUTABLE}"
        ${_build_arg} --vpi --Mdir "${_mdir}" --prefix "${FI_TB_TOP}"
        -CFLAGS "-g"
        -j "${FI_E2E_JOBS}"
        ${FI_SOURCES}
        ${_args}
      DEPENDS ${_depends}
      VERBATIM
    )
  endif()

  add_custom_target("${NAME}" DEPENDS "${_exe}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
  set("${NAME}_EXECUTABLE" "${_exe}" PARENT_SCOPE)
endfunction()

# fi_add_sim_run(<target>
#   [EXPECT_FAIL] [EXPECT_MISMATCH] [VEER_MODE]
#   [EXPECT_REGEX_MATCH <regex>] [EXPECT_VCD_GOLDENFILE <file_path>]
#   [EXECUTABLE <path>] [LOG <path>] [WORK_DIR <dir>]
#   [SIMULATION_DIR <dir>] [VCD_OUTPUT_PATH <path>] [ARGS <args...>]
#   [DEPENDS <deps...>])
#
# Runs a simulation executable through run_and_check_log.cmake.
#
# Required, explicit or implicit:
#   WORK_DIR          Command working directory.
#   SIMULATION_DIR   Used for executable/log defaults. Defaults to
#                    ${WORK_DIR}/obj_dir.
#
# Optional/implicit:
#   EXECUTABLE        Defaults to ${SIMULATION_DIR}/Vtop.
#   LOG               Defaults to ${SIMULATION_DIR}/run.log.
#   VCD_OUTPUT_PATH   VCD path produced by the simulation. Defaults to
#                     vlt_dump.vcd.
#   ARGS              Extra runtime arguments appended to EXECUTABLE.
#   EXPECT_FAIL       Allow the run command to fail.
#   EXPECT_REGEX_MATCH
#                     Require the run output to match the specified regex.
#   EXPECT_MISMATCH   Alias for EXPECT_FAIL and EXPECT_REGEX_MATCH "Mismatch".
#   EXPECT_VCD_GOLDENFILE <file_path>
#                     Require the run VCD output to be exactly the same as
#                     <file_path>.
#   VEER_MODE         Runs the simulation with VeeR testbench setup.
#
# The log is written by the script, but the CMake output is a .stamp file. The
# stamp is touched only after the script succeeds, so failed runs cannot leave a
# stale successful output just because run.log was written.
function(fi_add_sim_run NAME)
  set(options EXPECT_FAIL EXPECT_MISMATCH VEER_MODE)
  set(one_value_args EXECUTABLE LOG WORK_DIR SIMULATION_DIR VCD_OUTPUT_PATH EXPECT_VCD_GOLDENFILE TB_TOP EXPECT_REGEX_MATCH)
  set(multi_value_args ARGS DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_resolve_implicit_arg_with_default(SIMULATION_DIR "${FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(TB_TOP "${FI_E2E_TB_TOP_DEFAULT_FILENAME}" "")
  _fi_resolve_implicit_arg_with_default(EXECUTABLE "${FI_TB_TOP}" SIMULATION_DIR)
  _fi_resolve_implicit_arg_with_default(LOG run.log SIMULATION_DIR)
  _fi_resolve_implicit_arg_with_default(VCD_OUTPUT_PATH "${FI_E2E_VCD_OUTPUT_PATH_DEFAULT_FILENAME}" "")

  if(FI_EXPECT_MISMATCH)
    set(FI_EXPECT_FAIL "YES")
    set(FI_EXPECT_REGEX_MATCH "Mismatch")
  endif()

  set(_stamp "${FI_LOG}.stamp")
  set(_args)
  if(FI_EXPECT_FAIL)
    list(APPEND _args "-DFI_EXPECT_FAIL=ON")
  endif()
  if(FI_EXPECT_REGEX_MATCH)
    list(APPEND _args "-DFI_EXPECT_REGEX_MATCH=${FI_EXPECT_REGEX_MATCH}")
  endif()
  if(FI_EXPECT_VCD_GOLDENFILE)
    if(IS_ABSOLUTE "${FI_VCD_OUTPUT_PATH}")
      set(_actual_vcd "${FI_VCD_OUTPUT_PATH}")
    else()
      set(_actual_vcd "${FI_SIMULATION_DIR}/${FI_VCD_OUTPUT_PATH}")
    endif()
    set(_expect_vcd_arg "${_actual_vcd}\\;${FI_EXPECT_VCD_GOLDENFILE}")
    list(APPEND _args "-DFI_EXPECT_VCD_GOLDENFILE=${_expect_vcd_arg}")
  endif()
  set(_fi_command_arg "${FI_EXECUTABLE}")
  if(FI_ARGS)
    string(APPEND _fi_command_arg ";${FI_ARGS}")
  endif()

  # Special case for VeeR testing - custom step is required before running the simulation binary
  if(FI_VEER_MODE)
    add_custom_command(
      OUTPUT "${_stamp}"
      COMMAND cp "${E2E_VEER_DIR}/program.hex" "${FI_WORK_DIR}"
      COMMAND
        "${CMAKE_COMMAND}"
        "-DFI_COMMAND=${_fi_command_arg}"
        "-DFI_WORK_DIR=${FI_WORK_DIR}"
        "-DFI_LOG=${FI_LOG}"
        ${_args}
        -P "${FI_E2E_SCRIPT_DIR}/run_and_check_log.cmake"
      COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
      DEPENDS ${FI_DEPENDS} "${FI_EXECUTABLE}"
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT "${_stamp}"
      COMMAND
        "${CMAKE_COMMAND}"
        "-DFI_COMMAND=${_fi_command_arg}"
        "-DFI_WORK_DIR=${FI_WORK_DIR}"
        "-DFI_LOG=${FI_LOG}"
        ${_args}
        -P "${FI_E2E_SCRIPT_DIR}/run_and_check_log.cmake"
      COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
      DEPENDS ${FI_DEPENDS} "${FI_EXECUTABLE}"
      VERBATIM
    )
  endif()
  add_custom_target("${NAME}" DEPENDS "${_stamp}")
endfunction()

# fi_add_multi_campaign_runs(<target>
#   [PUBLIC_FLAT_RW] [CAMPAIGN_DIR <dir>] [LOG_DIR <dir>] [VLT_CONFIG <path>]
#   [WORK_DIR <dir>] [SIMULATION_DIR <dir>]
#   [VERILATOR_SOURCES <files...>] [DEPENDS <deps...>])
#
# Runs every campaign file in a directory through run_multi_campaigns.cmake.
# This is currently used by the worker-multiple scenario.
#
# Required, explicit or implicit:
#   WORK_DIR            Command working directory.
#   SIMULATION_DIR     Per-run output root. Defaults to ${WORK_DIR}/obj_dir.
#   VERILATOR_SOURCES  Defaults from SOURCES.
#
# Optional/implicit:
#   CAMPAIGN_DIR       Defaults to existing CAMPAIGN_DIR, or
#                      ${SIMULATION_DIR}/fault_campaign.
#   LOG_DIR            Defaults to existing LOG_DIR, or
#                      ${SIMULATION_DIR}/logs.
#   PUBLIC_FLAT_RW     Use Verilator's --public-flat-rw instead of VLT_CONFIG.
#
# The build-time script creates one Verilator build directory and run log per
# campaign, then fails with a list of campaigns that did not report "Mismatch".
function(fi_add_multi_campaign_runs NAME)
  set(options PUBLIC_FLAT_RW)
  set(one_value_args CAMPAIGN_DIR LOG_DIR WORK_DIR SIMULATION_DIR VLT_CONFIG)
  set(multi_value_args VERILATOR_SOURCES DEPENDS)
  cmake_parse_arguments(FI "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  _fi_resolve_implicit_arg_with_default(SIMULATION_DIR "${FI_E2E_VERILATOR_OUTPUT_DIR_DEFAULT_FILENAME}" WORK_DIR)
  _fi_resolve_implicit_arg_with_default(CAMPAIGN_DIR "${FI_E2E_CAMPAIGN_DIR_DEFAULT_FILENAME}" SIMULATION_DIR)
  _fi_resolve_implicit_arg_with_default(LOG_DIR logs SIMULATION_DIR)
  _fi_default_arg(FI_WORK_DIR WORK_DIR)
  _fi_default_arg(FI_VERILATOR_SOURCES SOURCES)

  set(_depends faultergeist-inject ${FI_DEPENDS} ${FI_VERILATOR_SOURCES})
  if(FI_PUBLIC_FLAT_RW)
    set(_public_flat_arg --public-flat-rw)
  else()
    _fi_resolve_implicit_arg_with_default(VLT_CONFIG "${FI_E2E_VLT_CONFIG_DEFAULT_FILENAME}" SIMULATION_DIR)
    set(_public_flat_arg "${FI_VLT_CONFIG}")
    list(APPEND _depends "${FI_VLT_CONFIG}")
  endif()

  set(_stamp "${FI_LOG_DIR}/${NAME}.stamp")
  add_custom_command(
    OUTPUT "${_stamp}"
    COMMAND
      "${CMAKE_COMMAND}"
      "-DVERILATOR_EXECUTABLE=${VERILATOR_EXECUTABLE}"
      "-DFI_JOBS=${FI_E2E_JOBS}"
      "-DFI_CAMPAIGN_DIR=${FI_CAMPAIGN_DIR}"
      "-DFI_LOG_DIR=${FI_LOG_DIR}"
      "-DFI_WORK_DIR=${FI_WORK_DIR}"
      "-DFI_FAULT_INJECTOR_SV=${FI_E2E_FAULT_INJECTOR_DIR}/faultergeist-inject.sv"
      "-DFI_FAULT_INJECTOR_LIB_DIR=$<TARGET_FILE_DIR:faultergeist-inject>"
      "-DFI_VERILATOR_SOURCES=$<JOIN:${FI_VERILATOR_SOURCES},;>"
      "-DFI_PUBLIC_FLAT_ARG=${_public_flat_arg}"
      -P "${FI_E2E_SCRIPT_DIR}/run_multi_campaigns.cmake"
    COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
    DEPENDS ${_depends}
    VERBATIM
  )
  add_custom_target("${NAME}" DEPENDS "${_stamp}")
  add_dependencies("${NAME}" "${FI_E2E_TOOLS_TARGET}")
endfunction()
