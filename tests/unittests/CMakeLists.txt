
if(NOT USE_EXTERNAL_CATCH2)
  message("Fetching local copy of Catch2 library for unit-tests...")
  Include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0)
  FetchContent_MakeAvailable(Catch2)
  set(CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras ${CMAKE_MODULE_PATH})
endif()

add_executable(k4SimDelphesTests   tests.cpp tests_edm4hepdelphesconverter.cpp)
target_compile_options(k4SimDelphesTests PRIVATE -Wno-c2y-extensions)
target_link_libraries(k4SimDelphesTests
                        DelphesEDM4HepConverter
                        EDM4HEP::edm4hep
                        ${DELPHES_LIBRARY}
                        podio::podio
                        ROOT::MathCore
                        ROOT::EG
                        Catch2::Catch2 Catch2::Catch2WithMain
                        )

include(CTest)
include(Catch)
catch_discover_tests(k4SimDelphesTests
                     WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                     DISCOVERY_MODE PRE_TEST
                     DL_PATHS ${PROJECT_BINARY_DIR}/converter
                     PROPERTIES ENVIRONMENT DELPHES_CARDS_DIR=${DELPHES_CARDS_DIR})
