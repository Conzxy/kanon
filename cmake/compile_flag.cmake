if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS  "/DWIN32 /D_WINDOWS /W0 /GR /EHsc /Zc:__cplusplus")
  set(CMAKE_CXX_FLAGS_DEBUG   "/MDd /Zi /Ob0 /Od /RTC1 /Zc:__cplusplus")
  set(CMAKE_CXX_FLAGS_RELEASE  "/MD /O2 /Ob2 /DNDEBUG /Zc:__cplusplus")
  set(CMAKE_CXX_FLAGS_MINSIZEREL  "/MD /O1 /Ob1 /DNDEBUG /Zc:__cplusplus")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DNDEBUG /Zc:__cplusplus")
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
  -Wno-deprecated-declarations
  # Allow lambda local variable to 
  # shadow th local variable of this function
  # contains lambda
  -Wno-shadow
  # Disable visibility hidden warning
  -Wno-attributes

  #-Wno-return-local-addr
  #-Wno-unused-parameter
  #-Wno-unused-function
  #-Wno-switch
  #-Wno-format-security
  -Wno-format-truncation
  # support INT2DOUBLE
  #-Wno-strict-aliasing
  # -Werror

  # make non-trivial(but like "trivial") class can reallocate
  -Wno-class-memaccess
  -Wno-implicit-fallthrough
  -Wno-maybe-uninitialized
  -Wwrite-strings # in fact, this is default specified
  -pthread
  # linker opt
  -rdynamic
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
