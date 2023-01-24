
if(BUILD_UNITTESTS)
  add_subdirectory(unittests)
endif()

add_executable(compare_delphes_converter_outputs src/compare_delphes_converter_outputs.cpp)
target_include_directories(compare_delphes_converter_outputs
  PRIVATE
  ${DELPHES_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/converter/src #for delphesHelpers
  ${CMAKE_SOURCE_DIR}/converter/include
  )
target_link_libraries(compare_delphes_converter_outputs PRIVATE EDM4HEP::edm4hep ${DELPHES_LIBRARY} podio::podioRootIO ROOT::Physics)

function(ADD_COMPARISON_TEST name converter)
  # Check if we have the standalone Delphes application available for the
  # desired input reader and only add the test if it is present
  string(REPLACE "_EDM4HEP" "" DELPHES_EXE_NAME ${converter})
  # Store the different possible executables in different cached variables to
  # check each individually, but not repeatedly check for the same ones again
  set(DELPHES_EXE "DELPHES_EXE_${DELPHES_EXE_NAME}")
  find_program(${DELPHES_EXE} "${DELPHES_EXE_NAME}" PATHS ${DELPHES_BINARY_DIR} $ENV{PATH})
  if(NOT ${DELPHES_EXE})
    message(WARNING "Cannot find delphes executable ${DELPHES_EXE_NAME} which is necessary to produce comparison output for test ${name}. Not adding this test")
    return()
  endif()

  add_test(NAME ${name}
    COMMAND bash -x ${CMAKE_SOURCE_DIR}/tests/testDriver.sh ${converter} ${ARGN})

  set_property(TEST ${name} PROPERTY ENVIRONMENT
    LD_LIBRARY_PATH=$<TARGET_FILE_DIR:DelphesEDM4HepConverter>:$<TARGET_FILE_DIR:EDM4HEP::edm4hepDict>:$<TARGET_FILE_DIR:podio::podioDict>:$<TARGET_FILE_DIR:ROOT::RIO>:$ENV{LD_LIBRARY_PATH}
    PATH=$<TARGET_FILE_DIR:${converter}>:${DELPHES_BINARY_DIR}:$ENV{PATH}
    COMPARE=$<TARGET_FILE:compare_delphes_converter_outputs>
    )
endfunction()

set(OUTPUT_CONFIG ${CMAKE_SOURCE_DIR}/examples/edm4hep_output_config.tcl)

# Get the delphes card from the Delphes installation
find_file(DELPHES_CARD delphes_card_IDEA.tcl PATHS ${DELPHES_CARDS_DIR})
if(DELPHES_CARD)
  if(BUILD_PYTHIA_READER)
    ADD_COMPARISON_TEST(PythiaConverter_ee_Z_bbbar DelphesPythia8_EDM4HEP ${DELPHES_CARD} ${OUTPUT_CONFIG} pythia_converter_output_ee_Z_bbbar.root ${CMAKE_SOURCE_DIR}/examples/data/ee_Z_bbbar_ecm91GeV.cmd)
  endif()
else()
  message(WARNING "Cannot find IDEA delphes card from the delphes installation. Cannot use it for testing")
endif()

#Use tester card for testing the ResonanceDecayUserHook
find_file(DELPHES_CARD delphes_card_IDEA.tcl PATHS ${DELPHES_CARDS_DIR})
if(DELPHES_CARD)
  if(BUILD_PYTHIA_READER)
    ADD_COMPARISON_TEST(PythiaResonanceDecayFilter_pp_hhbbyy DelphesPythia8_EDM4HEP ${DELPHES_CARD} ${OUTPUT_CONFIG} pythia_resonance_decay_filter_pp_hhbbyy_output.root ${CMAKE_SOURCE_DIR}/examples/data/tester_pwp8_pp_hh_5f_hhbbyy.cmd --no-delphes)
  endif()
else()
  message(WARNING "Cannot find IDEA delphes card from the delphes installation. Cannot use it for testing")
endif()


# Comparison test with the new FCC-hh card 
find_file(DELPHES_CARD_FCC card.tcl PATHS ${DELPHES_CARDS_DIR_LOCAL}) #try the new FCC-hh card

#need also the subdependencies of the card
find_file(DELPHES_CARD_FCC_TRACKRESO trackMomentumResolution.tcl PATHS ${DELPHES_CARDS_DIR_LOCAL})
find_file(DELPHES_CARD_FCC_MURESO muonMomentumResolutionVsP.tcl PATHS ${DELPHES_CARDS_DIR_LOCAL}) 

#and move them to local
if(DELPHES_CARD_FCC AND DELPHES_CARD_FCC_TRACKRESO AND DELPHES_CARD_FCC_MURESO)

  file(COPY ${DELPHES_CARD_FCC_TRACKRESO} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${DELPHES_CARD_FCC_MURESO} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  if(BUILD_PYTHIA_READER)
    ADD_COMPARISON_TEST(PythiaConverter_ggHH_100TeV_FCCCard DelphesPythia8_EDM4HEP ${DELPHES_CARD_FCC} ${OUTPUT_CONFIG} pythia_converter_output_ggHH_100TeV.root ${CMAKE_SOURCE_DIR}/examples/data/tester_pwp8_pp_hh_5f_hh_incl.cmd)
  endif()
else()
  message(WARNING "Cannot find FCC-hh delphes card from the delphes installation. Cannot use it for testing")
endif()

# Comparison test with the old FCC-hh card 
find_file(DELPHES_CARD_FCC_OLD FCChh.tcl PATHS ${DELPHES_CARDS_DIR} PATH_SUFFIXES FCC) 

#need also the subdependencies of the card
find_file(DELPHES_CARD_FCC_TRACKRESO_OLD momentumResolutionVsP.tcl PATHS ${DELPHES_CARDS_DIR} PATH_SUFFIXES FCC)
find_file(DELPHES_CARD_FCC_MURESO_OLD muonMomentumResolutionVsP.tcl PATHS ${DELPHES_CARDS_DIR} PATH_SUFFIXES FCC) 

#and move them to local
if(DELPHES_CARD_FCC_OLD AND DELPHES_CARD_FCC_TRACKRESO_OLD AND DELPHES_CARD_FCC_MURESO_OLD)

  file(COPY ${DELPHES_CARD_FCC_TRACKRESO_OLD} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${DELPHES_CARD_FCC_MURESO_OLD} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  if(BUILD_PYTHIA_READER)
    ADD_COMPARISON_TEST(PythiaConverter_ggHH_100TeV_oldFCChhCard DelphesPythia8_EDM4HEP ${DELPHES_CARD_FCC_OLD} ${OUTPUT_CONFIG} pythia_converter_output_ggHH_100TeV_oldCard.root ${CMAKE_SOURCE_DIR}/examples/data/tester_pwp8_pp_hh_5f_hh_incl.cmd)
  endif()
else()
  message(WARNING "Cannot find old FCC-hh delphes card from the delphes installation. Cannot use it for testing")
endif()

