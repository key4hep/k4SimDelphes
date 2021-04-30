
if(USE_EXTERNAL_CATCH2)
  find_package(Catch2 REQUIRED)
else()
  message("Fetching local copy of Catch2 library for unit-tests...")
  Include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        4b51d0dd3be90f4faf3c10f60d1923ad47a792d7)
  FetchContent_MakeAvailable(Catch2)
  set(CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras ${CMAKE_MODULE_PATH})
endif()


add_executable(k4SimDelphesTests   tests.cpp tests_edm4hepdelphesconverter.cpp)
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
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../)

