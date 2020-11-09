
# build executable and put it into bin/
add_executable(DelphesROOT_EDM4HEP src/DelphesROOT_EDM4HEP.cpp src/Delphes.cc)
target_link_libraries(DelphesROOT_EDM4HEP DelphesEDM4HepConverter ROOT::Physics)
install(TARGETS DelphesROOT_EDM4HEP DESTINATION bin)

if(BUILD_HEPMC_READER)
  # build executable and put it into bin/
  add_executable(DelphesHepMC_EDM4HEP src/DelphesHepMC_EDM4HEP.cpp src/Delphes.cc)
  target_link_libraries(DelphesHepMC_EDM4HEP DelphesEDM4HepConverter)
  install(TARGETS DelphesHepMC_EDM4HEP DESTINATION bin)
endif()

# build executable and put it into bin/
add_executable(DelphesSTDHEP_EDM4HEP src/DelphesSTDHEP_EDM4HEP.cpp src/Delphes.cc)
target_link_libraries(DelphesSTDHEP_EDM4HEP DelphesEDM4HepConverter)
install(TARGETS DelphesSTDHEP_EDM4HEP DESTINATION bin)

if(BUILD_PYTHIA_READER)
  find_package(Pythia8 REQUIRED)
  add_executable(DelphesPythia8_EDM4HEP src/DelphesPythia8_EDM4HEP.cpp src/Delphes.cc)
  target_include_directories(DelphesPythia8_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS})
  target_link_libraries(DelphesPythia8_EDM4HEP DelphesEDM4HepConverter ${PYTHIA8_LIBRARIES} ${DELPHES_LIBRARY})
  install(TARGETS DelphesPythia8_EDM4HEP DESTINATION bin)
endif()

if(BUILD_EVTGEN_READER)
  find_package(EvtGen REQUIRED)
  find_package(Pythia8 REQUIRED)
  if(PYTHIA8_VERSION VERSION_LESS 300)
    message("ERROR: Cannot build EVTGEN Reader. PYTHIA8300 or newer required. ")
  else()
    add_executable(DelphesPythia8EvtGen_EDM4HEP src/DelphesPythia8EvtGen_EDM4HEP.cpp src/Delphes.cc)
    target_include_directories(DelphesPythia8EvtGen_EDM4HEP PRIVATE ${PYTHIA8_INCLUDE_DIRS} ${EVTGEN_INCLUDE_DIR})
    target_link_libraries(DelphesPythia8EvtGen_EDM4HEP DelphesEDM4HepConverter ${PYTHIA8_LIBRARIES} ${DELPHES_LIBRARY} ${EVTGEN_LIBRARIES})
    install(TARGETS DelphesPythia8EvtGen_EDM4HEP DESTINATION bin)
  endif()
endif()


