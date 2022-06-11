##########################################################
# Get the kanon source files
##########################################################
file(GLOB_RECURSE KANON_BASE_SRC 
  string/*.cc
  log/*.cc
  process/*.cc
  thread/*.cc
  util/*.cc
  algo/*.cc
  buffer/*.cc
)

file(GLOB_RECURSE KANON_NET_SRC net/*.cc)

message(STATUS "kanon base source files: ${KANON_BASE_SRC}")
message(STATUS "kanon net source files: ${KANON_NET_SRC}")

##########################################################
# Generate kanon base and net library
# User can set generate shared libraries as following command
# cd build && cmake .. -D BUILD_SHARED_LIBS=ON
##########################################################
GenLib(kanon_base ${KANON_BASE_SRC})
GenLib(kanon_net ${KANON_NET_SRC})
target_link_libraries(kanon_net kanon_base)

###########################################################
# Install the library to /usr/lib
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

install(TARGETS kanon_base kanon_net DESTINATION lib)

set(KANON_INSTALL_INCLUDE_PREFIX "/usr/include/kanon")
file(GLOB LOG_INCLUDE log/*.h)
file(GLOB PROCESS_INCLUDE process/*.h)
file(GLOB STRING_INCLUDE string/*.h)
file(GLOB THREAD_INCLUDE thread/*.h)
file(GLOB UTIL_INCLUDE util/*.h)
install(FILES ${LOG_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/log)
install(FILES ${PROCESS_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/process)
install(FILES ${STRING_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/string)
install(FILES ${THREAD_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/thread)
install(FILES ${UTIL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/util)
install(FILES ${NET_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net)
file(GLOB NET_INCLUDE net/*.h)
file(GLOB NET_POLL_INCLUDE net/poll/*.h)
file(GLOB NET_TIMER_INCLUDE net/timer/*.h)

install(FILES ${NET_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net)
install(FILES ${NET_POLL_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net/poll)
install(FILES ${NET_TIMER_INCLUDE} DESTINATION ${KANON_INSTALL_INCLUDE_PREFIX}/net/timer)

set(BUILD_PROTOBUF OFF CACHE BOOL "Control if build protobuf module")
set(BUILD_PROTOBUF_RPC OFF CACHE BOOL "Control if build protobuf-rpc module")

if (${BUILD_PROTOBUF} OR ${BUILD_PROTOBUF_RPC})
  add_subdirectory(protobuf)
endif (${BUILD_PROTOBUF} OR ${BUILD_PROTOBUF_RPC})

if (${BUILD_PROTOBUF_RPC})
  add_subdirectory(rpc)
endif (${BUILD_PROTOBUF_RPC})
