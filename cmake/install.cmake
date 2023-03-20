function (conzxy_install)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "PROJECT;NAMESPACE" "TARGETS")

  include(GNUInstallDirs)
  set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/${CONZXY_PROJECT}Config.cmake.in")
  set(project_config_out "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}Config.cmake")
  set(config_targets_file "${CONZXY_PROJECT}ConfigTargets.cmake")
  set(version_config_file "${CMAKE_CURRENT_BINARY}/${CONZXY_PROJECT}ConfigVersion.cmake")
  set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${CONZXY_PROJECT}")

  install(
    TARGETS ${CONZXY_TARGETS}
    EXPORT ${CONZXY_PROJECT}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  export(
      TARGETS ${CONZXY_TARGETS}
      NAMESPACE ${CONZXY_NAMESPACE}::
      FILE "${CMAKE_CURRENT_BINARY}/${config_targets_file}")
  install(EXPORT ${CONZXY_PROJECT} DESTINATION "${export_dest_dir}" NAMESPACE ${CONZXY_NAMESPACE}:: FILE ${config_targets_file})

  include(CMakePackageConfigHelpers)
  configure_package_config_file("${project_config_in}" "${project_config_out}" INSTALL_DESTINATION "${export_dest_dir}")
  write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
  install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")
endfunction ()
