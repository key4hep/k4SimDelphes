# - Use CMake's module to help generating relocatable config files
include(CMakePackageConfigHelpers)

# - Versioning
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/k4SimDelphesConfigVersion.cmake
  VERSION ${k4SimDelphes_VERSION}
  COMPATIBILITY SameMajorVersion)

# - Install time config and target files
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/k4SimDelphesConfig.cmake.in
  "${PROJECT_BINARY_DIR}/k4SimDelphesConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/k4SimDelphes"
  PATH_VARS
    CMAKE_INSTALL_BINDIR
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
  )

# - install and export
install(FILES
  "${PROJECT_BINARY_DIR}/k4SimDelphesConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/k4SimDelphesConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/k4SimDelphes"
  )
install(EXPORT k4SimDelphesTargets
  NAMESPACE k4SimDelphes::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/k4SimDelphes"
  )

