


#--- Add main module
file(GLOB _sources src/*.cpp)
gaudi_add_module(k4SimDelphesPlugins
                 SOURCES ${_sources}
                 LINK k4FWCore::k4FWCore
                      k4FWCore::k4Interface
                      DelphesEDM4HepConverter
                      Gaudi::GaudiKernel
                      EDM4HEP::edm4hep
                      ROOT::Core
                )

install(DIRECTORY examples DESTINATION share/${PROJECT_NAME})

#--- Testing
if(BUILD_TESTING)

  #--- The genConf directory has been renamed to genConfDir in Gaudi 35r1
  #--- See https://gitlab.cern.ch/gaudi/Gaudi/-/merge_requests/1158
  set(GAUDI_GENCONF_DIR "genConfDir")
  if (${Gaudi_VERSION} VERSION_LESS 35.1)
    set(GAUDI_GENCONF_DIR "genConf")
  endif()

  if(TARGET k4Gen::k4Gen)
    add_test(NAME k4SimDelphesAlgBasicTest
             WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples/options
             COMMAND  k4run k4simdelphesalg.py)
    # set up gaudi environment for tests
    set_property(TEST k4SimDelphesAlgBasicTest APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}:${PROJECT_BINARY_DIR}/framework/k4SimDelphes:$<TARGET_FILE_DIR:k4Gen::k4Gen>:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>:$ENV{LD_LIBRARY_PATH}"
    "PYTHONPATH=$<TARGET_FILE_DIR:k4Gen::k4Gen>/../python:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>/../python:${PROJECT_BINARY_DIR}/framework/k4SimDelphes/${GAUDI_GENCONF_DIR}:$ENV{PYTHONPATH}"
    )

    add_test(NAME k4SimDelphesAlgPythiaTest
             WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples/options
             COMMAND  k4run k4simdelphesalg_pythia.py)
    # set up gaudi environment for tests
    set_property(TEST k4SimDelphesAlgPythiaTest APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}:${PROJECT_BINARY_DIR}/framework/k4SimDelphes:$<TARGET_FILE_DIR:k4Gen::k4Gen>:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>:$ENV{LD_LIBRARY_PATH}"
    "PYTHONPATH=$<TARGET_FILE_DIR:k4Gen::k4Gen>/../python:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>/../python:${PROJECT_BINARY_DIR}/framework/k4SimDelphes/${GAUDI_GENCONF_DIR}:$ENV{PYTHONPATH}"
    )

  else()
    message(WARNING "k4Gen not found, disabling some test!")
  endif()
endif()
