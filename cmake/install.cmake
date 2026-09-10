# SPDX-License-Identifier: Apache-2.0

include(GNUInstallDirs)

install(TARGETS faultergeist-gen faultergeist-inject
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(FILES "${PROJECT_SOURCE_DIR}/src/FaultInjector/faultergeist-inject.sv"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/faultergeist"
)

file(RELATIVE_PATH FAULTERGEIST_PC_PREFIX
  "/${CMAKE_INSTALL_LIBDIR}/pkgconfig" "/")
string(REGEX REPLACE "/$" "" FAULTERGEIST_PC_PREFIX "${FAULTERGEIST_PC_PREFIX}")
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/faultergeist.pc.in"
  "${PROJECT_BINARY_DIR}/faultergeist.pc"
  @ONLY
)
install(FILES "${PROJECT_BINARY_DIR}/faultergeist.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
