##########################################################
# Get the kanon source files
##########################################################
file(GLOB_RECURSE KANON_BASE_SRC 
  string/*.cc
  log/*.cc
  mem/*.cc
  process/*.cc
  thread/*.cc
  util/*.cc
  algo/*.cc
  buffer/*.cc
)

file(GLOB_RECURSE KANON_NET_SRC net/*.cc)

#message(STATUS "kanon base source files: ${KANON_BASE_SRC}")
#message(STATUS "kanon net source files: ${KANON_NET_SRC}")

##########################################################
# Generate kanon base and net library
# User can set generate shared libraries as following command
# cd build && cmake .. -D BUILD_SHARED_LIBS=ON
##########################################################
# In other platform(eg. windows), generating base library only.
if (UNIX AND NOT APPLE)
  message(STATUS "Kanon on unix/linux/solaris/... platform")
  set(KANON_ON_UNIX 1)
else ()
  set(AKNON_ON_UNIX 0)
endif ()

if (WIN32)
  message(STATUS "Kanon on windows")
endif ()

file(GLOB LOG_INCLUDE log/*.h)
file(GLOB PROCESS_INCLUDE process/*.h)
file(GLOB STRING_INCLUDE string/*.h)
file(GLOB THREAD_INCLUDE thread/*.h)
file(GLOB UTIL_INCLUDE util/*.h)
file(GLOB BUFFER_INCLUDE buffer/*.h)
file(GLOB ZSTL_INCLUDE zstl/*.h)
file(GLOB ALGO_INCLUDE algo/*.h)
file(GLOB MEM_INCLUDE mem/*.h)
file(GLOB ALGO_FORWARD_LIST_INCLUDE algo/forward_list/*.h)
set(KANON_BASE_INCLUDE ${LOG_INCLUDE} ${PROCESS_INCLUDE} ${STRING_INCLUDE}
    ${THREAD_INCLUDE} ${UTIL_INCLUDE} ${BUFFER_INCLUDE} ${ZSTL_INCLUDE} ${ALGO_INCLUDE}
    ${MEM_INCLUDE} ${ALGO_FORWARD_LIST_INCLUDE})

file(GLOB NET_INCLUDE net/*.h)
file(GLOB NET_POLL_INCLUDE net/poll/*.h)
file(GLOB NET_TIMER_INCLUDE net/timer/*.h)
file(GLOB NET_CONNECTION_INCLUDE net/connection/*.h)
file(GLOB XXHASH_INCLUDE ${THIRD_PARTY_DIR}/xxHash/*.h)
set(KANON_NET_INCLUDE ${NET_INCLUDE} ${NET_POLL_INCLUDE} ${NET_TIMER_INCLUDE} ${NET_CONNECTION_INCLUDE}
    ${XXHASH_INCLUDE})

GenLib(kanon_base ${KANON_BASE_SRC})
target_include_directories(kanon_base PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

if (KANON_ON_UNIX)
  GenLib(kanon_net ${KANON_NET_SRC})
  target_link_libraries(kanon_net PRIVATE kanon_base)
  target_link_libraries(kanon_net PRIVATE xxHash::xxhash)
  target_include_directories(kanon_net PUBLIC
      "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>"
      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
endif ()

add_library(kanon::kanon_base ALIAS kanon_base)
add_library(kanon::kanon_net ALIAS kanon_net)

###########################################################
# Install the headers to /usr/include/*
#
# User can install these files by following command:
# cmake --install */kanon/build
# Also, you specify the install prefix instead of /usr/lib:
# cmake --install */kanon/build --prefix /usr(or other place)
############################################################
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "The install prefix of libraries" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

set(KANON_INSTALL_INCLUDE_PREFIX "${CMAKE_INSTALL_INCLUDEDIR}/kanon")

if (KANON_INSTALL)
  install(FILES ${LOG_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/log)
  install(FILES ${PROCESS_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/process)
  install(FILES ${STRING_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/string)
  install(FILES ${THREAD_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/thread)
  install(FILES ${UTIL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/util)
  install(FILES ${BUFFER_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/buffer)
  install(FILES ${ZSTL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/zstl)
  install(FILES ${ALGO_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/algo)
  install(FILES ${ALGO_FORWARD_LIST_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/algo/forward_list)

  install(FILES ${NET_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net)
  install(FILES ${NET_CONNECTION_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net/connection)
  install(FILES ${NET_POLL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net/poll)
  install(FILES ${NET_TIMER_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net/timer)
  install(FILES ${XXHASH_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/third-party/xxHash)
  install(TARGETS kanon_base DESTINATION lib)
  if (${KANON_ON_UNIX})
    install(TARGETS kanon_net DESTINATION lib)
  endif ()

endif()

if (KANON_BUILD_PROTOBUF OR KANON_BUILD_PROTOBUF_RPC)
  add_subdirectory(protobuf)
endif ()

if (KANON_BUILD_PROTOBUF_RPC)
  add_subdirectory(rpc)
endif ()
