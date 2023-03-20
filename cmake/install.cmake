function (conzxy_install)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "PROJECT;NAMESPACE" "TARGETS")

  include(GNUInstallDirs)
  set(project_config_in_file "${CMAKE_CURRENT_LIST_DIR}/cmake/${CONZXY_PROJECT}Config.cmake.in")
  set(project_config_out_file "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}Config.cmake")
  set(config_targets_filename "${CONZXY_PROJECT}ConfigTargets.cmake")
  set(config_version_file "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}ConfigVersion.cmake")
  set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${CONZXY_PROJECT}")
    
  # Specify rules about targets
  install(
    TARGETS ${CONZXY_TARGETS}
    EXPORT ${CONZXY_PROJECT}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  )
    
  export(
      EXPORT ${CONZXY_PROJECT}
      NAMESPACE ${CONZXY_NAMESPACE}::
      FILE "${CMAKE_CURRENT_BINARY_DIR}/${config_targets_filename}")
  # Install ConfigTargets.cmake
  install(EXPORT ${CONZXY_PROJECT} 
      DESTINATION "${export_dest_dir}"
      NAMESPACE ${CONZXY_NAMESPACE}::
      FILE ${config_targets_filename})
    
  # Install Config.cmake and ConfigVersion.cmake
  include(CMakePackageConfigHelpers)
  # (<in> <out>)
  configure_package_config_file("${project_config_in_file}" "${project_config_out_file}" 
      INSTALL_DESTINATION "${export_dest_dir}")
  write_basic_package_version_file("${config_version_file}" COMPATIBILITY SameMajorVersion)
  install(FILES "${project_config_out_file}" "${config_version_file}" DESTINATION "${export_dest_dir}")
endfunction ()
