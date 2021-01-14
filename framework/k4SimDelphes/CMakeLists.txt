

file(GLOB _sources src/*.cpp)
gaudi_add_module(k4SimDelphesPlugins
                 SOURCES ${_sources}
                 LINK k4FWCore::k4FWCorePlugins
                      k4FWCore::k4Interface
                      Gaudi::GaudiAlgLib
                      Gaudi::GaudiKernel
                      EDM4HEP::edm4hep
                      ROOT::Core
                      ROOT::Hist
                                )
