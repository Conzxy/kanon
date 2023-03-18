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

macro (GenLib lib)
  #if (NOT ${BUILD_SHARED_LIBS})
  if (${BUILD_STATIC_LIBS})
    message(STATUS "Build static library: ${lib}")
    add_library(${lib} STATIC ${ARGN})
    set_target_properties(${lib}
      PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
  else ()
    message(STATUS "Build shared library: ${lib}")
    add_library(${lib} SHARED ${ARGN})
    set_target_properties(${lib}
      PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
  endif (${BUILD_STATIC_LIBS})
endmacro ()

# Check if kanon is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED KANON_MAIN_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(KANON_MAIN_PROJECT ON)
    else()
        set(KANON_MAIN_PROJECT OFF)
    endif()
endif()