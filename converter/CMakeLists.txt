

add_library(DelphesEDM4HepConverter SHARED src/DelphesEDM4HepConverter.cc)
target_include_directories(DelphesEDM4HepConverter
  SYSTEM PUBLIC ${DELPHES_INCLUDE_DIRS}
  )
target_include_directories(DelphesEDM4HepConverter
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

target_link_libraries(DelphesEDM4HepConverter
                        EDM4HEP::edm4hep
                        ${DELPHES_LIBRARY}
                        podio::podio
                        ROOT::MathCore
                        ROOT::EG
                        )

install(DIRECTORY include/
        TYPE INCLUDE)

install(TARGETS DelphesEDM4HepConverter
        EXPORT k4SimDelphesTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION include/k4SimDelphes
        COMPONENT dev)
