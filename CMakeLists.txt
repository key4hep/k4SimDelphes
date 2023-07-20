cmake_minimum_required(VERSION 3.3)

project(k4SimDelphes)

#--- Declare options -----------------------------------------------------------

option(BUILD_PYTHIA_READER "Build the EDM4hep plugin for Gaudi" ON)
option(BUILD_EVTGEN_READER "Build the DELPHES plugin for Gaudi" ON)
option(BUILD_HEPMC_READER "Build the DELPHES plugin for Gaudi" ON)
option(BUILD_FRAMEWORK "Build the DELPHES plugin for Gaudi" ON)
option(K4SIMDELPHES_DOCUMENTATION "Whether or not to create doxygen doc target." OFF)
option(ENABLE_CPACK "Whether or not to use cpack config" OFF)
option(USE_EXTERNAL_CATCH2 "If off, build the catch2 unittest framework locally" ON)

#--- Declare project version --------------------------------------------------
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 5 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

SET( ${PROJECT_NAME}_VERSION
  "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}" )


#--- Such that the binaries in install are properly linked --------------------
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE CACHE BOOL "RPATH USE LINK PATH")

#--- Declare C++ Standard -----------------------------------------------------
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
if(NOT CMAKE_CXX_STANDARD MATCHES "17")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()
message (STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")


#-- Use GNU-style hierarchy for installing build products ----------------------
include(GNUInstallDirs)

#--- setup build type (and default) if supported -------------------------------
if (NOT CMAKE_CONFIGURATION_TYPES)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo
      CACHE STRING "Choose the type of build, options are: None|Release|MinSizeRel|Debug|RelWithDebInfo" FORCE)
  else()
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
      CACHE STRING "Choose the type of build, options are: None|Release|MinSizeRel|Debug|RelWithDebInfo" FORCE)
  endif()
endif()

#--- add version files ---------------------------------------------------------
configure_file(${PROJECT_SOURCE_DIR}/k4SimDelphesVersion.h
               ${PROJECT_BINARY_DIR}/k4SimDelphesVersion.h )
install(FILES ${PROJECT_BINARY_DIR}/k4SimDelphesVersion.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/k4SimDelphes )


#--- add CMake infrastructure --------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(cmake/k4SimDelphesCreateConfig.cmake)

#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
              DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- enable unit testing capabilities ------------------------------------------
include(CTest)

#--- enable CPack --------------------------------------------------------------
if(ENABLE_CPACK)
  include(cmake/k4SimDelphesCPack.cmake)
endif()

#--- target for Doxygen documentation ------------------------------------------
if(K4SIMDELPHES_DOCUMENTATION)
  include(cmake/k4SimDelphesDoxygen.cmake)
endif()


#--- Non-optional Dependencies ------------------------------------------------
find_package(Delphes REQUIRED)
find_package(EDM4HEP REQUIRED)

#--- optional dependencies
if(BUILD_PYTHIA_READER OR BUILD_EVTGEN_READER)
  find_package(Pythia8 REQUIRED)

  if(BUILD_EVTGEN_READER)
    if(PYTHIA8_VERSION VERSION_LESS 300)
      message("ERROR: Cannot build EVTGEN Reader. PYTHIA8300 or newer required. ")
    endif()

    find_package(EvtGen REQUIRED)
  endif()
endif()

#--- Code Sub-directories -----------------------------------------------------
add_subdirectory(converter)
add_subdirectory(standalone)
add_subdirectory(examples)


if(BUILD_FRAMEWORK)
  find_package(Gaudi REQUIRED) # Do not depend on implicit find via k4FWCore
  find_package(k4FWCore REQUIRED)
  if (BUILD_TESTING)
    find_package(k4Gen REQUIRED) # Finds Gaudi, etc. again
  endif()

  add_subdirectory(framework)
endif()

if(BUILD_TESTING)
  option(BUILD_UNITTESTS "Build the unittest for k4SimDelphes" ON)
  if(BUILD_UNITTESTS)
    if(USE_EXTERNAL_CATCH2)
      find_package(Catch2 REQUIRED)
    endif()
  endif()

  add_subdirectory(tests)
endif()


#--- create uninstall target ---------------------------------------------------
include(cmake/k4SimDelphesUninstall.cmake)
