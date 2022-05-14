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

register_rpc_test(simple_server simple.pb.cc )
register_rpc_test(simple_client simple.pb.cc )
register_rpc_test(simple_client2 simple.pb.cc)
register_rpc_test(simple_client3 simple.pb.cc)
register_rpc_test(simple_clients simple.pb.cc)
register_rpc_test(echorpc_client echo.pb.cc)
register_rpc_test(echorpc_server echo.pb.cc)