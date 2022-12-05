# build executable and put it into bin/
add_executable(DelphesROOT_EDM4HEP src/DelphesROOT_EDM4HEP.cpp)
target_link_libraries(DelphesROOT_EDM4HEP DelphesEDM4HepConverter podio::podioRootIO ROOT::Physics)
install(TARGETS DelphesROOT_EDM4HEP DESTINATION bin)

if(BUILD_HEPMC_READER)
  # build executable and put it into bin/
  add_executable(DelphesHepMC_EDM4HEP src/DelphesHepMC_EDM4HEP.cpp)
  target_link_libraries(DelphesHepMC_EDM4HEP DelphesEDM4HepConverter podio::podioRootIO)
  install(TARGETS DelphesHepMC_EDM4HEP DESTINATION bin)
endif()

# build executable and put it into bin/
add_executable(DelphesSTDHEP_EDM4HEP src/DelphesSTDHEP_EDM4HEP.cpp)
target_link_libraries(DelphesSTDHEP_EDM4HEP DelphesEDM4HepConverter podio::podioRootIO ${DELPHES_LIBRARY})
install(TARGETS DelphesSTDHEP_EDM4HEP DESTINATION bin)

if(BUILD_PYTHIA_READER)
  add_executable(DelphesPythia8_EDM4HEP src/DelphesPythia8_EDM4HEP.cpp)
  target_include_directories(DelphesPythia8_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS})
  target_link_libraries(DelphesPythia8_EDM4HEP DelphesEDM4HepConverter podio::podioRootIO ${PYTHIA8_LIBRARIES} ${DELPHES_LIBRARY})
  install(TARGETS DelphesPythia8_EDM4HEP DESTINATION bin)
endif()

if(BUILD_EVTGEN_READER)
  add_library(PythiaEvtGen_Interface SHARED src/PythiaEvtGen_Interface.cc)
  target_include_directories(PythiaEvtGen_Interface PRIVATE ${PYTHIA8_INCLUDE_DIRS} )
  target_link_libraries(PythiaEvtGen_Interface ${PYTHIA8_LIBRARIES}
                        EvtGen::EvtGen EvtGen::EvtGenExternal)
  install(TARGETS PythiaEvtGen_Interface DESTINATION ${CMAKE_INSTALL_LIBDIR})

  add_executable(DelphesPythia8EvtGen_EDM4HEP src/DelphesPythia8EvtGen_EDM4HEP.cpp)
  add_executable(DelphesPythia8EvtGen_EDM4HEP_k4Interface src/DelphesPythia8EvtGen_EDM4HEP_k4Interface.cpp)

  target_include_directories(DelphesPythia8EvtGen_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS} )
  target_link_libraries(DelphesPythia8EvtGen_EDM4HEP
                        DelphesEDM4HepConverter
                        podio::podioRootIO
                        ${PYTHIA8_LIBRARIES}
                        ${DELPHES_LIBRARY}
                        EvtGen::EvtGen
                        EvtGen::EvtGenExternal
                        )
  install(TARGETS DelphesPythia8EvtGen_EDM4HEP DESTINATION bin)

  target_include_directories(DelphesPythia8EvtGen_EDM4HEP_k4Interface PRIVATE ${PYTHIA8_INCLUDE_DIRS})
  target_link_libraries(DelphesPythia8EvtGen_EDM4HEP_k4Interface
                        PythiaEvtGen_Interface
                        DelphesEDM4HepConverter
                        podio::podioRootIO
                        ${PYTHIA8_LIBRARIES}
                        ${DELPHES_LIBRARY}
                        EvtGen::EvtGen
                        EvtGen::EvtGenExternal
                        )
  install(TARGETS DelphesPythia8EvtGen_EDM4HEP_k4Interface DESTINATION bin)
endif()
