


#--- Add main module
file(GLOB _sources src/*.cpp)
gaudi_add_module(k4SimDelphesPlugins
                 SOURCES ${_sources}
                 LINK k4FWCore::k4FWCore
                      k4FWCore::k4Interface
                      DelphesEDM4HepConverter
                      Gaudi::GaudiAlgLib
                      Gaudi::GaudiKernel
                      EDM4HEP::edm4hep
                      ROOT::Core
                      ROOT::Hist
                )

install(DIRECTORY examples DESTINATION share/${PROJECT_NAME})

#--- Testing
if(BUILD_TESTING)
  find_package(k4Gen)
  get_target_property(k4Gen_lib k4Gen::k4Gen LOCATION)
  get_filename_component(k4Gen_loc ${k4Gen_lib} DIRECTORY)

  if(TARGET k4Gen::k4Gen)
    add_test(NAME k4SimDelphesAlgBasicTest
             WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples/options
             COMMAND  k4run k4simdelphesalg.py)
    # set up gaudi environment for tests
    set_property(TEST k4SimDelphesAlgBasicTest APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}:${CMAKE_BINARY_DIR}/framework/k4SimDelphes:${k4Gen_loc}:$ENV{LD_LIBRARY_PATH}")
    set_property(TEST k4SimDelphesAlgBasicTest APPEND PROPERTY ENVIRONMENT "PYTHONPATH=${k4Gen_loc}/../python:${CMAKE_BINARY_DIR}/framework/k4SimDelphes/genConf:$ENV{PYTHONPATH}")

  else()
    message(WARNING "k4Gen not found, disabling some test!")
  endif()
endif()