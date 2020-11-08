

add_library(DelphesEDM4HepConverter SHARED src/DelphesEDM4HepConverter.cc)
target_include_directories(DelphesEDM4HepConverter SYSTEM PUBLIC ${DELPHES_INCLUDE_DIRS})
target_include_directories(DelphesEDM4HepConverter
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
target_link_libraries(DelphesEDM4HepConverter edm4hep ${DELPHES_LIBRARY} podio::podioRootIO ROOT::MathCore ROOT::EG ROOT::Physics)
set_target_properties(DelphesEDM4HepConverter PROPERTIES PUBLIC_HEADER "include/k4simdelphes/DelphesEDM4HepConverter.h")
install(TARGETS DelphesEDM4HepConverter 
        EXPORT K4SIMDELPHESTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include/k4simdelphes
        COMPONENT dev)
