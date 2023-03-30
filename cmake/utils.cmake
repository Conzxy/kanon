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

function (GenLib lib)
  #if (NOT ${BUILD_SHARED_LIBS})
  if (KANON_BUILD_STATIC_LIBS)
    message(STATUS "Build static library: ${lib}")
    add_library(${lib} STATIC ${ARGN})
  else ()
    message(STATUS "Build shared library: ${lib}")
    add_library(${lib} SHARED ${ARGN})
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
