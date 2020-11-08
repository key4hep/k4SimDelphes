
# build all executables and put them into bin/
add_executable(DelphesROOT_EDM4HEP src/DelphesROOT_EDM4HEP.cpp src/Delphes.cc)
target_link_libraries(DelphesROOT_EDM4HEP DelphesEDM4HepConverter ROOT::Physics)
install(TARGETS DelphesROOT_EDM4HEP DESTINATION bin)

# build all executables and put them into bin/
add_executable(DelphesHepMC_EDM4HEP src/DelphesHepMC_EDM4HEP.cpp src/Delphes.cc)
target_link_libraries(DelphesHepMC_EDM4HEP DelphesEDM4HepConverter)
install(TARGETS DelphesHepMC_EDM4HEP DESTINATION bin)

# build all executables and put them into bin/
add_executable(DelphesSTDHEP_EDM4HEP src/DelphesSTDHEP_EDM4HEP.cpp src/Delphes.cc)
target_link_libraries(DelphesSTDHEP_EDM4HEP DelphesEDM4HepConverter)
install(TARGETS DelphesSTDHEP_EDM4HEP DESTINATION bin)

find_package(Pythia8)
if(Pythia8_FOUND)
  add_executable(DelphesPythia8_EDM4HEP src/DelphesPythia8_EDM4HEP.cpp src/Delphes.cc)
  target_include_directories(DelphesPythia8_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS})
  target_link_libraries(DelphesPythia8_EDM4HEP DelphesEDM4HepConverter ${PYTHIA8_LIBRARIES} ${DELPHES_LIBRARY})
  install(TARGETS DelphesPythia8_EDM4HEP DESTINATION bin)
  find_package(EvtGen)
  
  add_executable(DelphesPythia8EvtGen_EDM4HEP src/DelphesPythia8EvtGen_EDM4HEP.cpp src/Delphes.cc)
  target_include_directories(DelphesPythia8EvtGen_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS} ${EVTGEN_INCLUDE_DIR})
  target_link_libraries(DelphesPythia8EvtGen_EDM4HEP DelphesEDM4HepConverter ${PYTHIA8_LIBRARIES} ${DELPHES_LIBRARY} ${EVTGEN_LIBRARIES})
  install(TARGETS DelphesPythia8EvtGen_EDM4HEP DESTINATION bin)

endif()


