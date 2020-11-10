cmake_minimum_required(VERSION 3.3)

project(K4SIMDELPHES)

#--- Declare options -----------------------------------------------------------

option(BUILD_PYTHIA_READER "Build the EDM4hep plugin for DD4hep" ON)
option(BUILD_EVTGEN_READER "Build the DELPHES plugin for DD4hep" ON)
option(BUILD_HEPMC_READER "Build the DELPHES plugin for DD4hep" ON)
option(BUILD_FRAMEWORK "Build the DELPHES plugin for DD4hep" OFF)
option(K4SIMDELPHES_DOCUMENTATION "Whether or not to create doxygen doc target." OFF)
option(ENABLE_CPACK "Whether or not to use cpack config" OFF)

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

SET( ${PROJECT_NAME}_VERSION  "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}" )


set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

if(NOT CMAKE_CXX_STANDARD MATCHES "17")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()

message (STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

#--- add version files ---------------------------------------------------------

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/K4SIMDELPHESVersion.h
               ${CMAKE_CURRENT_BINARY_DIR}/K4SIMDELPHESVersion.h )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/K4SIMDELPHESVersion.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/k4simdelphes )

# - Use GNU-style hierarchy for installing build products
include(GNUInstallDirs)

#--- add CMake infrastructure --------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(cmake/K4SIMDELPHESCreateConfig.cmake)

#--- add license files ---------------------------------------------------------
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
              DESTINATION ${CMAKE_INSTALL_DOCDIR})

#--- enable unit testing capabilities ------------------------------------------
include(CTest)

#--- enable CPack --------------------------------------------------------------
if(ENABLE_CPACK)
  include(cmake/K4SIMDELPHESCPack.cmake)
endif()

#--- target for Doxygen documentation ------------------------------------------
if(K4SIMDELPHES_DOCUMENTATION)
  include(cmake/K4SIMDELPHESDoxygen.cmake)
endif()


#--- Non-optional Dependencies ------------------------------------------

find_package(Delphes)
find_package(EDM4HEP)

add_subdirectory(converter)
add_subdirectory(standalone)
if(BUILD_FRAMEWORK)
  add_subdirectory(framework)
endif()


#--- create uninstall target ---------------------------------------------------
include(cmake/K4SIMDELPHESUninstall.cmake)
