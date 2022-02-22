# - Locate pythia6 library
# Defines:
#
#  PYTHIA8_FOUND
#  PYTHIA8_VERSION
#  PYTHIA8_INCLUDE_DIR
#  PYTHIA8_INCLUDE_DIRS (not cached)
#  PYTHIA8_LIBRARY
#  PYTHIA8_hepmcinterface_LIBRARY
#  PYTHIA8_lhapdfdummy_LIBRARY
#  PYTHIA8_LIBRARIES (not cached) : for PYTHIA8_VERSION < 200 includes 3 libraries above; not to be used if lhapdf is used

set(_pythia8_dirs
    ${PYTHIA8}
    $ENV{PYTHIA8}
    ${PYTHIA8_DIR}
    $ENV{PYTHIA8_DIR}
    ${PYTHIA8_ROOT_DIR}
)

find_path(PYTHIA8_INCLUDE_DIR
          NAMES Pythia8/Pythia.h
          HINTS ${_pythia8_dirs}
          PATH_SUFFIXES include include/Pythia8
          NO_DEFAULT_PATH)

find_path(PYTHIA8_XML_DIR Version.xml
          HINTS ${_pythia8_dirs}
          PATH_SUFFIXES xmldoc share/Pythia8/xmldoc
          NO_DEFAULT_PATH)

message(STATUS "xml path: ${PYTHIA8_XML_DIR}")

file(READ ${PYTHIA8_XML_DIR}/Version.xml versionstr)
string(REGEX REPLACE ".*Pythia:versionNumber.*default.*[0-9][.]([0-9]+).*" "\\1" PYTHIA8_VERSION "${versionstr}")

message(STATUS "pythia8 version extracted: ${PYTHIA8_VERSION}")

find_library(PYTHIA8_LIBRARY
             NAMES pythia8 Pythia8
             HINTS ${_pythia8_dirs}
             PATH_SUFFIXES lib
             NO_DEFAULT_PATH)

if(PYTHIA8_VERSION VERSION_LESS 200)
  find_library(PYTHIA8_hepmcinterface_LIBRARY
               NAMES hepmcinterface pythia8tohepmc
               HINTS ${_pythia8_dirs}
               PATH_SUFFIXES lib
               NO_DEFAULT_PATH)

  find_library(PYTHIA8_lhapdfdummy_LIBRARY
               NAMES lhapdfdummy
               HINTS ${_pythia8_dirs}
               PATH_SUFFIXES lib
               NO_DEFAULT_PATH)

  set(PYTHIA8_LIBRARIES ${PYTHIA8_LIBRARY} ${PYTHIA8_hepmcinterface_LIBRARY} ${PYTHIA8_lhapdfdummy_LIBRARY})
else()
  set(PYTHIA8_LIBRARIES ${PYTHIA8_LIBRARY})
endif()

set(PYTHIA8_INCLUDE_DIRS ${PYTHIA8_INCLUDE_DIR} ${PYTHIA8_INCLUDE_DIR}/Pythia8 )

# handle the QUIETLY and REQUIRED arguments and set PYTHIA8_FOUND to TRUE if
# all listed variables are TRUE

INCLUDE(FindPackageHandleStandardArgs)
if(PYTHIA8_VERSION VERSION_LESS 200)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Pythia8 DEFAULT_MSG PYTHIA8_INCLUDE_DIR PYTHIA8_LIBRARY PYTHIA8_hepmcinterface_LIBRARY PYTHIA8_lhapdfdummy_LIBRARY)
  mark_as_advanced(PYTHIA8_FOUND PYTHIA8_INCLUDE_DIR PYTHIA8_LIBRARY PYTHIA8_hepmcinterface_LIBRARY PYTHIA8_lhapdfdummy_LIBRARY PYTHIA8_VERSION_CHECK)
else()
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Pythia8 DEFAULT_MSG PYTHIA8_INCLUDE_DIR PYTHIA8_LIBRARY)
  mark_as_advanced(PYTHIA8_FOUND PYTHIA8_INCLUDE_DIR PYTHIA8_LIBRARY PYTHIA8_VERSION_CHECK)
endif()
