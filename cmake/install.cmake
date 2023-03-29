function (conzxy_install)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "PROJECT;NAMESPACE" "TARGETS")

  include(GNUInstallDirs)
  set(project_config_in_file "${CMAKE_CURRENT_LIST_DIR}/cmake/${CONZXY_PROJECT}Config.cmake.in")
  set(project_config_out_file "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}Config.cmake")
  set(config_targets_filename "${CONZXY_PROJECT}ConfigTargets.cmake")
  set(config_version_file "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}ConfigVersion.cmake")
  set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${CONZXY_PROJECT}")
    
  # Specify rules of targets
  install(
    TARGETS ${CONZXY_TARGETS}
    EXPORT ${CONZXY_PROJECT}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )

  # Export the ConfigTargets.cmake to build directory
  # To make user use find_package() successfully but don't install, 
  # you can use export(PACKAGE <name>) command to register build dir to
  # ~/.cmake/packages/
  export(
      EXPORT ${CONZXY_PROJECT}
      NAMESPACE ${CONZXY_NAMESPACE}::
      FILE "${CMAKE_CURRENT_BINARY_DIR}/${config_targets_filename}")

  # Generate and Install ConfigTargets.cmake
  install(EXPORT ${CONZXY_PROJECT} 
      DESTINATION "${export_dest_dir}"
      NAMESPACE ${CONZXY_NAMESPACE}::
      FILE ${config_targets_filename})
    
  include(CMakePackageConfigHelpers)
  # Generate Config.cmake and ConfigVersion.cmake
  # (<in> <out>)
  configure_package_config_file("${project_config_in_file}" "${project_config_out_file}" 
      INSTALL_DESTINATION "${export_dest_dir}")
  write_basic_package_version_file("${config_version_file}" COMPATIBILITY SameMajorVersion)
  message(STATUS "Config file path = ${project_config_out_file}")
  message(STATUS "ConfigVersion file path = ${config_version_file}")
  # Install Config.cmake and ConfigVersion.cmake
  install(FILES "${project_config_out_file}" "${config_version_file}" DESTINATION "${export_dest_dir}")
endfunction ()
