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
 -Wno-switch
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

set(KANON_BASE_SRC ${STRING_SRC} ${LOG_SRC} ${THREAD_SRC} ${PROCESS_SRC} ${TIME_SRC})

aux_source_directory(${KANON_SOURCE_DIR}/net NET_SRC)

set(KANON_NET_SRC ${NET_SRC})

file(GLOB_RECURSE KANON_SRC ${KANON_SOURCE_DIR}/*.cc)

file(GLOB_RECURSE ALGO_INCLUDE ${KANON_SOURCE_DIR}/algo/*.h)
file(GLOB_RECURSE LOG_INCLUDE ${KANON_SOURCE_DIR}/log/*.h)
file(GLOB_RECURSE NET_INCLUDE ${KANON_SOURCE_DIR}/net/*.h)
file(GLOB_RECURSE PROCESS_INCLUDE ${KANON_SOURCE_DIR}/process/*.h)
file(GLOB_RECURSE STRING_INCLUDE ${KANON_SOURCE_DIR}/string/*.h)
file(GLOB_RECURSE THREAD_INCLUDE ${KANON_SOURCE_DIR}/thread/*.h)
file(GLOB_RECURSE TIME_INCLUDE ${KANON_SOURCE_DIR}/time/*.h)
file(GLOB_RECURSE UTIL_INCLUDE ${KANON_SOURCE_DIR}/util/*.h)

message(STATUS "THREAD_INCLUDE: ${THREAD_INCLUDE}")

set(ncdm "[0-9a-zA-Z_-]")

list(FILTER KANON_SRC EXCLUDE REGEX "${ncdm}*test${ncdm}*.cc")
list(FILTER KANON_SRC EXCLUDE REGEX "${ncdm}*bench${ncdm}*.cc")
# list(FILTER KANON_INCLUDE EXCLUDE REGEX "${ncdm}*test${ncdm}*.h")

set(ALL_SRC ${KANON_BASE_SRC} ${KANON_NET_SRC})

add_subdirectory(${KANON_SOURCE_DIR}/util)
add_subdirectory(${KANON_SOURCE_DIR}/net)
add_subdirectory(${KANON_SOURCE_DIR}/string)
add_subdirectory(${KANON_SOURCE_DIR}/thread)
add_subdirectory(${CMAKE_SOURCE_DIR}/example)
add_subdirectory(${KANON_SOURCE_DIR}/log)
add_subdirectory(${KANON_SOURCE_DIR}/algo)
add_subdirectory(${KANON_SOURCE_DIR}/process)
add_subdirectory(${KANON_SOURCE_DIR}/time)


# target_include_directories(kanon_base PRIVATE ${KANON_SOURCE_DIR})
# target_include_directories(kanon_net PRIVATE ${KANON_SOURCE_DIR})

add_library(kanon_base SHARED ${KANON_BASE_SRC})
# set_target_properties(kanon_base PROPERTIES PUBLIC_HEADER 
# kanon/log/*.h kanon/process/*.h kanon/string/*.h kanon/thread/*.h kanon/time/*.h)

add_library(kanon_net SHARED ${KANON_NET_SRC})
# set_target_properties(kanon_net PROPERTIES PUBLIC_HEADER net/*.h)

# install(TARGETS kanon_base kanon_net
# LIBRARY DESTINATION lib
# PUBLIC_HEADER DESTINATION include)
install(TARGETS kanon_base kanon_net DESTINATION lib)

set(KANON_INSTALL_INCLUDE_PREFIX include/kanon)

install(FILES ${ALGO_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/algo)
install(FILES ${LOG_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/log)
install(FILES ${NET_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net)
install(FILES ${PROCESS_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/process)
install(FILES ${STRING_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/string)
install(FILES ${THREAD_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/thread)
install(FILES ${TIME_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/time)
install(FILES ${UTIL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/util)
