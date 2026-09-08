# SPDX-License-Identifier: Apache-2.0

add_custom_target(${FI_E2E_PDK_TARGET})
foreach(lib_archive lib_file IN ZIP_LISTS FI_E2E_ASAP7_LIBERTY_ARCHIVES FI_E2E_LIB_FILES)
  if(NOT EXISTS "${lib_file}")
    get_filename_component(_fi_e2e_lib_file_dir ${lib_file} DIRECTORY)
    get_filename_component(_fi_e2e_lib_file_name ${lib_file} NAME_WE)
    file(MAKE_DIRECTORY "${_fi_e2e_lib_file_dir}")
    file(ARCHIVE_EXTRACT
      INPUT "${lib_archive}"
      DESTINATION "${_fi_e2e_lib_file_dir}"
      TOUCH
    )
    add_custom_target("${_fi_e2e_lib_file_name}_target" DEPENDS ${lib_file})
    add_dependencies(${FI_E2E_PDK_TARGET} "${_fi_e2e_lib_file_name}_target")
  endif()
endforeach()

set(LIB_FILES "${FI_E2E_LIB_FILES}")

# Worker
add_subdirectory(e2e/worker-basic)
add_subdirectory(e2e/worker-multiple)
add_subdirectory(e2e/worker-synth-hierarchical)
add_subdirectory(e2e/worker-synth-regression)

# VeeR
add_subdirectory(e2e/veer-basic)
add_subdirectory(e2e/coverage)
