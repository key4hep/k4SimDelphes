
#### k4SimDelphes: Delphes integrated in the Gaudi framework

# dependencies
find_package(Gaudi)
find_package(k4FWCore)

add_subdirectory(k4SimDelphes)

install(EXPORT ${PROJECT_NAME}Targets
  NAMESPACE ${PROJECT_NAME}::
  FILE "${PROJECT_NAME}Targets.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/"
  )

gaudi_install(CMAKE cmake/${PROJECT_NAME}Config.cmake)
