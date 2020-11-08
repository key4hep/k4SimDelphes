# - Use CMake's module to help generating relocatable config files
include(CMakePackageConfigHelpers)

# - Versioning
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/K4SIMDELPHESConfigVersion.cmake
  VERSION ${K4SIMDELPHES_VERSION}
  COMPATIBILITY SameMajorVersion)

# - Install time config and target files
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/K4SIMDELPHESConfig.cmake.in
  "${PROJECT_BINARY_DIR}/K4SIMDELPHESConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/K4SIMDELPHES"
  PATH_VARS
    CMAKE_INSTALL_BINDIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
  )

# - install and export
install(FILES
  "${PROJECT_BINARY_DIR}/K4SIMDELPHESConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/K4SIMDELPHESConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/K4SIMDELPHES"
  )
install(EXPORT K4SIMDELPHESTargets
  NAMESPACE K4SIMDELPHES::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/K4SIMDELPHES"
  )

