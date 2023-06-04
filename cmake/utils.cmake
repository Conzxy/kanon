function(kanon_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/kanon/version.h" file_contents)
    string(REGEX MATCH "KANON_VER_MAJOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from kanon/version.h")
    endif()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "KANON_VER_MINOR ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from kanon/version.h")
    endif()

    set(ver_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "KANON_VER_PATCH ([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from kanon/version.h")
    endif()
    set(ver_patch ${CMAKE_MATCH_1})

    set(KANON_VERSION_MAJOR ${ver_major} PARENT_SCOPE)
    set(KANON_VERSION_MINOR ${ver_minor} PARENT_SCOPE)
    set(KANON_VERSION_PATCH ${ver_patch} PARENT_SCOPE)
    set(KANON_VERSION "${ver_major}.${ver_minor}.${ver_patch}" PARENT_SCOPE)
endfunction()

function (kanon_gen_lib lib)
  #if (NOT ${BUILD_SHARED_LIBS})
  if (KANON_BUILD_STATIC_LIBS)
    message(STATUS "Build static library: ${lib}")
    add_library(${lib} STATIC ${ARGN})
    
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(archive_output_path "${CMAKE_BINARY_DIR}/kanon/lib/debug")
    else ()
      set(archive_output_path "${CMAKE_BINARY_DIR}/kanon/lib/release")
    endif ()

    message(STATUS "${lib} output dir: ${archive_output_path}")
    set_target_properties(${lib}
     PROPERTIES
     ARCHIVE_OUTPUT_DIRECTORY
     ${archive_output_path})
  else ()
    message(STATUS "Build shared library: ${lib}")
    add_library(${lib} SHARED ${ARGN})
    # Follow the windows convention
    # *.dll to bin directory
    if (WIN32)
      set(so_output_dir bin)
    else ()
      set(so_output_dir lib)
    endif ()

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(shared_output_path "${CMAKE_BINARY_DIR}/kanon/${so_output_dir}/debug")
    else ()
      set(shared_output_path "${CMAKE_BINARY_DIR}/kanon/${so_output_dir}/release")
    endif ()
      
    message(STATUS "${lib} output dir: ${shared_output_path}")
    
    if (WIN32)
      # In windows, CMake think it is RUNTIME output instead of LIBRARY
      set_target_properties(${lib}
       PROPERTIES
       RUNTIME_OUTPUT_DIRECTORY 
       ${shared_output_path})

      # In windows, need consider *.Lib file
      # Don't put *.dll and *.Lib in same directory
      set(dll_lib_output_dir ${shared_output_path}/lib)
      message(STATUS "DLL lib output dir: ${dll_lib_output_dir}")
      set_target_properties(${lib}
       PROPERTIES
       ARCHIVE_OUTPUT_DIRECTORY 
       ${dll_lib_output_dir})
    else ()
      set_target_properties(${lib}
       PROPERTIES
       LIBRARY_OUTPUT_DIRECTORY 
       ${shared_output_path})
    endif ()
  endif ()

  message(STATUS "Lib ${lib} Sources: ${ARGN}")
endfunction ()

function (conzxy_copy_dll_to_target_dir)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "DLL_PATH;TARGET" "")
  add_custom_command(TARGET ${CONZXY_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CONZXY_DLL_PATH}"
    "$<TARGET_FILE_DIR:${CONZXY_TARGET}>")  
endfunction ()

# Check if kanon is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED KANON_MAIN_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(KANON_MAIN_PROJECT ON)
    else()
        set(KANON_MAIN_PROJECT OFF)
    endif()
endif()
