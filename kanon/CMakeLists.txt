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

file(GLOB_RECURSE KANON_NET_SRC ${PROJECT_SOURCE_DIR}/third-party/xxHash/xxhash.c net/*.cc)

#message(STATUS "kanon base source files: ${KANON_BASE_SRC}")
#message(STATUS "kanon net source files: ${KANON_NET_SRC}")

##########################################################
# Generate kanon base and net library
# User can set generate shared libraries as following command
# cd build && cmake .. -D BUILD_SHARED_LIBS=ON
##########################################################
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

#aux_source_directory(linux/core KANON_LINUX_CORE_SRC)
#aux_source_directory(./win/core KANON_WIN_CORE_SRC)
file(GLOB_RECURSE KANON_LINUX_CORE_SRC linux/core/*.cc)
file(GLOB_RECURSE KANON_WIN_CORE_SRC win/core/*.cc)
file(GLOB_RECURSE KANON_LINUX_NET_SRC linux/net/*.cc)
file(GLOB_RECURSE KANON_WIN_NET_SRC win/net/*.cc)

# This must be a absolute path here!
set(KANON_LINUX_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/core)
set(KANON_WIN_CORE_INCLUDE_DIR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/win/core)
set(KANON_LINUX_NET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/linux/net)
set(KANON_WIN_NET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/win/net)

set(KANON_LIB_POPULATE_KEYWORD PUBLIC)

if (KANON_ON_WIN)
  list(APPEND KANON_BASE_SRC ${KANON_WIN_CORE_SRC})
  list(APPEND KANON_NET_SRC ${KANON_WIN_NET_SRC})
endif ()

if (KANON_ON_UNIX)
  list(APPEND KANON_BASE_SRC ${KANON_LINUX_CORE_SRC})
  list(APPEND KANON_NET_SRC ${KANON_LINUX_NET_SRC})
endif ()

GenLib(kanon_base ${KANON_BASE_SRC})
target_include_directories(kanon_base ${KANON_LIB_POPULATE_KEYWORD}
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if (NOT KANON_BUILD_STATIC_LIBS)
  target_compile_definitions(kanon_base PRIVATE KANON_BUILD_CORE_SHARED
                                        INTERFACE KANON_LINK_CORE_SHARED)
endif ()
if (KANON_ON_WIN)
  target_link_libraries(kanon_base PUBLIC ws2_32)
  set_target_properties(kanon_base PROPERTIES LINKER_LANGUAGE CXX)
endif ()

GenLib(kanon_net ${KANON_NET_SRC})
target_link_libraries(kanon_net PUBLIC kanon_base)
#target_link_libraries(kanon_net PRIVATE xxHash::xxhash)
target_include_directories(kanon_net ${KANON_LIB_POPULATE_KEYWORD}
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
if (NOT KANON_BUILD_STATIC_LIBS)
  target_compile_definitions(kanon_net INTERFACE KANON_LINK_NET_SHARED
                                       PRIVATE KANON_BUILD_NET_SHARED)

endif ()
if (KANON_ON_WIN)
  target_link_libraries(kanon_net PUBLIC ws2_32)
  set_target_properties(kanon_net PROPERTIES LINKER_LANGUAGE CXX)
endif ()

###########################################################
# Install the headers to /usr/include/*
############################################################
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
endif()

if (KANON_BUILD_PROTOBUF OR KANON_BUILD_PROTOBUF_RPC)
  find_package(Protobuf REQUIRED)
  add_subdirectory(protobuf)
endif ()

if (KANON_BUILD_PROTOBUF_RPC)
  add_subdirectory(rpc)
endif ()
