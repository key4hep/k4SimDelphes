


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

    add_test(NAME k4SimDelphesAlgPythiaTest
             WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples/options
             COMMAND  k4run k4simdelphesalg_pythia.py)
    set_tests_properties(k4SimDelphesAlgPythiaTest PROPERTIES FIXTURES_SETUP k4SimDelphesAlgPythiaOutput)

    # The reco-MC links in the framework output have to be filled and point to the input MCParticles
    add_test(NAME k4SimDelphesAlgPythiaRecoMCLinkCheck
             WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples/options
             COMMAND python3 ${PROJECT_SOURCE_DIR}/tests/scripts/check_reco_mc_links.py output_k4SimDelphes_pythia.root)
    set_tests_properties(k4SimDelphesAlgPythiaRecoMCLinkCheck PROPERTIES FIXTURES_REQUIRED k4SimDelphesAlgPythiaOutput)

    # Set up the Gaudi environment for the tests. The local build has to come first in all paths, otherwise
    # a k4SimDelphes that is already in the stack (e.g. in the nightlies) is picked up instead
    set(_local_lib_dirs "$<TARGET_FILE_DIR:k4SimDelphesPlugins>:$<TARGET_FILE_DIR:DelphesEDM4HepConverter>")
    set(_test_environment
      "LD_LIBRARY_PATH=${_local_lib_dirs}:$<TARGET_FILE_DIR:k4Gen::k4Gen>:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>:$ENV{LD_LIBRARY_PATH}"
      "GAUDI_PLUGIN_PATH=${_local_lib_dirs}:$ENV{GAUDI_PLUGIN_PATH}"
      "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${GAUDI_GENCONF_DIR}:$<TARGET_FILE_DIR:k4Gen::k4Gen>/../python:$<TARGET_FILE_DIR:k4FWCore::k4FWCore>/../python:$ENV{PYTHONPATH}"
    )
    set_tests_properties(k4SimDelphesAlgBasicTest k4SimDelphesAlgPythiaTest k4SimDelphesAlgPythiaRecoMCLinkCheck
                         PROPERTIES ENVIRONMENT "${_test_environment}")

  else()
    message(WARNING "k4Gen not found, disabling some test!")
  endif()
endif()
