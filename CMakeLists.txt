cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(kanon)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

 
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS  "/DWIN32 /D_WINDOWS /W0 /GR /EHsc")
  set(CMAKE_CXX_FLAGS_DEBUG   "/MDd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_RELEASE  "/MD /O2 /Ob2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_MINSIZEREL  "/MD /O1 /Ob1 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DNDEBUG")
else ()
  # FYI https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og")

  # FYI https://gcc.gnu.org/onlinedocs/gcc-5.5.0/gcc/Link-Options.html#Link-Options
  # FYI https://gcc.gnu.org/onlinedocs/gcc-5.5.0/gcc/Warning-Options.html#Warning-Options

  set(CXX_FLAGS 
  -Wall
  -Wextra
  -Wno-return-local-addr
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-switch
  -Wno-format-security
  # support INT2DOUBLE
  -Wno-strict-aliasing
  # -Werror
  # make non-trivial(but like "trivial") class can reallocate
  -Wno-class-memaccess
  -Wno-implicit-fallthrough
  -Wconversion
  -Wshadow
  -Wno-maybe-uninitialized
  -Wwrite-strings # in fact, this is default specified
  -pthread
      
  # linker opt
  # -rdynamic
  # machine opt
  -march=native
  )

  # Clang和GCC有些选项是不通用的
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(REMOVE_ITEM CXX_FLAGS "-Wno-return-local-addr")
    list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
    list(APPEND CXX_FLAGS "-Wthread-safety")
  endif()

  string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
endif ()

set(CMAKE_CXX_STANDARD 11)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "CMAKE_CXX_FLAGS_DEBUG")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif ()

include_directories(${PROJECT_SOURCE_DIR})

set(KANON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/kanon)
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third-party)

# I don't want to use the builtin variable ${BUILD_SHARED_LIBS}
# since it you don't set it to ON explicitly, then the NOT 
# ${BUILD_SHARED_LIBS} will be false, and generated libraries 
# are static. That's not I wanted behavior. I want a variable,
# it can build shared libraries default even though I don't set 
# it explitly, and the build of static libraries is an option.
set(BUILD_STATIC_LIBS OFF CACHE BOOL "Build static libraries")

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

# message(STATUS "BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_ALL_TESTS = ${BUILD_ALL_TESTS}")
message(STATUS "BUILD_ALL_EXAMPLES = ${BUILD_ALL_EXAMPLES}")

add_subdirectory(kanon)

set (BUILD_TESTS OFF CACHE BOOL "Build tests(Consider test directory)")

if (${BUILD_TESTS})
  add_subdirectory(test)
endif ()

add_subdirectory(example)
