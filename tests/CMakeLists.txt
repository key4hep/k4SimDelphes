
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
    LD_LIBRARY_PATH=$<TARGET_FILE_DIR:DelphesEDM4HepConverter>:$<TARGET_FILE_DIR:EDM4HEP::edm4hepDict>:$<TARGET_FILE_DIR:podio::podioDict>:$ENV{LD_LIBRARY_PATH}
    PATH=$<TARGET_FILE_DIR:${converter}>:${DELPHES_BINARY_DIR}:$ENV{PATH}
    COMPARE=$<TARGET_FILE:compare_delphes_converter_outputs>
    )
endfunction()

# Verbatim copy of the one shipped with delphes, except that we fix the
# RandomSeed to get repeatable outcomes and also limit the number of events
set(DELPHES_CARD ${CMAKE_SOURCE_DIR}/tests/data/delphes_card_IDEA.tcl)
set(OUTPUT_CONFIG ${CMAKE_SOURCE_DIR}/examples/edm4hep_output_config.tcl)

if(BUILD_PYTHIA_READER)
  ADD_COMPARISON_TEST(PythiaConverter DelphesPythia8_EDM4HEP ${DELPHES_CARD} ${OUTPUT_CONFIG} pythia_converter_output_ee_Z_bbbar.root ${CMAKE_SOURCE_DIR}/examples/data/ee_Z_bbbar_ecm91GeV.cmd)

  # Only run tests using the TrackCovariance if it is actually available
  if (DELPHES_TRACK_COV_HEADER)
    set(DELPHES_TRK_COV_CARD ${CMAKE_SOURCE_DIR}/tests/data/delphes_card_IDEAtrkCov.tcl)
    ADD_COMPARISON_TEST(PythiaConverter_TrkCov DelphesPythia8_EDM4HEP ${DELPHES_TRK_COV_CARD} ${OUTPUT_CONFIG} pythia_converter_output_ee_Z_ccbar.root ${CMAKE_SOURCE_DIR}/examples/data/ee_Z_bbbar_ecm91GeV.cmd)
  endif()
endif()
