add_executable(compare_delphes_converter_outputs src/compare_delphes_converter_outputs.cpp)
target_include_directories(compare_delphes_converter_outputs
  PRIVATE
  ${DELPHES_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/converter/include
  )
target_link_libraries(compare_delphes_converter_outputs PRIVATE EDM4HEP::edm4hep ${DELPHES_LIBRARY} podio::podioRootIO ROOT::Physics)

function(ADD_COMPARISON_TEST name converter)
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
  ADD_COMPARISON_TEST(PythiaConverter DelphesPythia8_EDM4HEP ${DELPHES_CARD} ${OUTPUT_CONFIG} pythia_converter_output.root ${CMAKE_SOURCE_DIR}/examples/data/ee_Z_bbbar_ecm91GeV.cmd)
endif()
