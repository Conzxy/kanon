cmake_minimum_required(VERSION 3.10)
# never set compiler in CMakeLists.txt
# beacuse it need set before project and enable_language

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#set(CMAKE_C_COMPILER "/usr/local/bin/clang")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
project(kanon)

#enable_testing()

#if (NOT CMAKE_BUILD_TYPE)
#	set(CMAKE_BUILD_TYPE "Release")
#endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og")

set(CXX_FLAGS 
 -g
 -Wall
 -Wextra
 -Wno-return-local-addr
 -Wno-unused-parameter
 -Wno-unused-function
 -Werror
 #-Wconversion
 #-Wshadow
 -Wwrite-strings # in fact, this is default specified
 #cxx standard 
 -std=c++11
 -pthread
	
 # linker opt
 -rdynamic
 # machine opt
 -march=native
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	list(REMOVE_ITEM CXX_FLAGS "-Wno-return-local-addr")
	list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
	list(APPEND CXX_FLAGS "-Wthread-safety")
endif()


string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}; BUILD_TYPE: ${CAMKE_BUILD_TYPE}")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/bin)

#link_libraries("/usr/local/lib/libgtest.a")

include_directories(${CMAKE_SOURCE_DIR})

set(KANON_SOURCE_DIR ${CMAKE_SOURCE_DIR}/kanon)

aux_source_directory(${KANON_SOURCE_DIR}/string STRING_SRC)
aux_source_directory(${KANON_SOURCE_DIR}/log LOG_SRC)
aux_source_directory(${KANON_SOURCE_DIR}/thread THREAD_SRC)
aux_source_directory(${KANON_SOURCE_DIR}/process PROCESS_SRC)
aux_source_directory(${KANON_SOURCE_DIR}/time TIME_SRC)

set(KANON_BASE_SRC
${STRING_SRC} ${LOG_SRC} ${THREAD_SRC} ${PROCESS_SRC} ${TIME_SRC})

aux_source_directory(${KANON_SOURCE_DIR}/net NET_SRC)


file(GLOB_RECURSE KANON_SRC ${KANON_SOURCE_DIR}/*.cc)

set(ncdm "[0-9a-zA-Z_-]")

list(FILTER KANON_SRC EXCLUDE REGEX "${ncdm}*test${ncdm}*.cc")

message(STATUS "string src: ${STRING_SRC}")
message(STATUS "log src: ${LOG_SRC}")
message(STATUS "thread src: ${THREAD_SRC}")
message(STATUS "KANON_SRC: ${KANON_SRC}")

set(ALL_SRC 
	${LOG_SRC} ${STRING_SRC} ${THREAD_SRC} ${NET_SRC})

add_subdirectory(${KANON_SOURCE_DIR}/net)
#add_subdirectory(string)
add_subdirectory(${KANON_SOURCE_DIR}/thread)
add_subdirectory(${CMAKE_SOURCE_DIR}/example)
add_subdirectory(${KANON_SOURCE_DIR}/log)
add_subdirectory(${KANON_SOURCE_DIR}/algo)
add_subdirectory(${KANON_SOURCE_DIR}/process)
add_subdirectory(${KANON_SOURCE_DIR}/time)
