cmake_minimum_required(VERSION 3.10)
# Generate compile_commands.json
# to make clangd works.
# !WARNING
# This only useful for `Ninja` and `Makefile` generator
# For other generator, CMake ignore it, i.e., do nothing.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Include useful module
include(cmake/utils.cmake)
include(cmake/compile_flag.cmake)
include(cmake/build_type.cmake)
include(cmake/cache_var.cmake)

# Extract the version from version.h in the top level of include directory
kanon_extract_version()

project(kanon VERSION ${KANON_VERSION})
message(STATUS "Building kanon: ${KANON_VERSION}")

# Add third-party before kanon
add_subdirectory(third-party)
# Add source directory before install
add_subdirectory(kanon)

if (KANON_INSTALL)
  message(STATUS "Generating (kanon) install files...")
  include(GNUInstallDirs)
  set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/kanonConfig.cmake.in")
  set(project_config_out "${CMAKE_CURRENT_BINARY}/kanonConfig.cmake")
  set(config_targets_file "kanonConfigTargets.cmake")
  set(version_config_file "${CMAKE_CURRENT_BINARY}/kanonConfigVersion.cmake")
  set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/kanon")
    
  set(kanon_export_targets kanon_base kanon_net)
  if (KANON_BUILD_PROTOBUF)
    list(APPEND kanon_export_targets kanon_protobuf)
  endif ()
  if (KANON_BUILD_PROTOBUF_RPC)
    list(APPEND kanon_export_targets kanon_protobuf_rpc)
  endif ()

  install(
    TARGETS ${kanon_export_targets}
    EXPORT kanon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  export(
      TARGETS ${kanon_export_targets}
      NAMESPACE kanon::
      FILE "${CMAKE_CURRENT_BINARY}/${config_targets_file}")
  install(EXPORT kanon DESTINATION ${export_dest_dir} NAMESPACE kanon:: FILE ${config_targets_file})

  include(CMakePackageConfigHelpers)
  configure_package_config_file("${project_config_in}" "${project_config_out}" INSTALL_DESTINATION "${export_dest_dir}")
  write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
  install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")
endif ()
    
# Tests
if (KANON_TESTS)
  add_subdirectory(test)
endif ()

# Examples
if (KANON_EXAMPLES)
  add_subdirectory(example)
endif ()
