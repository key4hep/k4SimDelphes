#--- install the default output configuration to the shared directory which will
#--- be picked up by the spack installation as $K4SIMDELPHES

install(FILES edm4hep_output_config.tcl
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME})
