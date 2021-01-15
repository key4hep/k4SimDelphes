


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


#--- Testing
add_test(NAME k4SimDelphesAlgBasicTest
         WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
         COMMAND ${CMAKE_BINARY_DIR}/run k4run tests/options/k4simdelphesalg.py)

