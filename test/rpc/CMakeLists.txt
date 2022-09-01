function (register_rpc_test filename_we)
  if (${BUILD_ALL_TESTS})
    add_executable(${filename_we} ${filename_we}.cc simple.pb.cc)
  else ()
    message(STATUS "${filename_we} excluded from target all")
    add_executable(${filename_we} EXCLUDE_FROM_ALL ${filename_we}.cc ${ARGN})
  endif (${BUILD_ALL_TESTS})
  target_link_libraries(${filename_we} protobuf_rpc kanon_base kanon_net)
  set_target_properties(${filename_we}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    COMMAND ${filename_we})
  target_include_directories(${filename_we} PRIVATE ${CMAKE_BINARY_DIR})
endfunction ()

if (NOT ${BUILD_ALL_TESTS})
  message(STATUS "[The rpc tests are excluded from build of target all]")
endif (NOT ${BUILD_ALL_TESTS}) 

file(GLOB RPC_TEST_SRC *.cc)
file(GLOB PB_SRC pb/*.pb.cc)

foreach (rpc_test_src ${RPC_TEST_SRC})
  get_filename_component(rpc_test_filename ${rpc_test_src} NAME_WE)
  message(STATUS ${rpc_test_filename})
  register_rpc_test(${rpc_test_filename} ${PB_SRC})
endforeach()
